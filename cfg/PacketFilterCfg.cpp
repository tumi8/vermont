#include "PacketFilterCfg.h"

#include <sampler/regExFilter.h>
#include <sampler/stringFilter.h>
#include <sampler/SystematicSampler.h>
#include "common/msg.h"


#include <cassert>


PacketFilterCfg::PacketFilterCfg(XMLElement* elem)
	: CfgHelper<FilterModule, PacketFilterCfg>(elem, "filter")
{
	if (!elem)
		return;
	
	XMLNode::XMLSet<XMLElement*> set = elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		Cfg* c;
		XMLElement* e = *it;

		if (e->matches("countBased")) {
			msg(MSG_FATAL, "Filter: Creating count based sampler");
			c = new PacketCountFilterCfg(e);
		} else if (e->matches("stringBased")) {
			msg(MSG_FATAL, "Filter: Creating string based sampler");
			c = new PacketStringFilterCfg(e);
		} else if (e->matches("regexBased")) {
			msg(MSG_FATAL, "Filter: Creating regex based sampler");
			c = new PacketRegexFilterCfg(e);
		} else if (e->matches("timeBased")) {
			msg(MSG_FATAL, "Filter: Creating time based sampler");
		} else if (e->matches("next")) { // ignore next
			continue;
		} else {
			msg(MSG_FATAL, "Unkown packet filter %s\n", e->getName().c_str());
			continue;
		}

		subCfgs.push_back(c);
	}
}

PacketFilterCfg::~PacketFilterCfg()
{
	for (std::vector<Cfg*>::iterator it = subCfgs.begin();
	     it != subCfgs.end();
	     it++) {
		delete *it;
	}
}

PacketFilterCfg* PacketFilterCfg::create(XMLElement* e)
{
	assert(e);
	assert(e->getName() == getName());
	return new PacketFilterCfg(e);
}

FilterModule* PacketFilterCfg::createInstance()
{
	instance = new FilterModule();
	for (std::vector<Cfg*>::iterator it = subCfgs.begin();
	     it != subCfgs.end();
	     it++) {
		instance->addProcessor(reinterpret_cast<PacketProcessor*>((*it)->getInstance()));
	}
	return instance;
}

bool PacketFilterCfg::deriveFrom(PacketFilterCfg* old)
{
	// check for same number of filters
	if (subCfgs.size() != old->subCfgs.size())
		return false;

	for (size_t i = 0; i < subCfgs.size(); i++) {
		if (!subCfgs[i]->deriveFrom(old->subCfgs[i]))
			return false;
	}

	transferInstance(old);
	return true;
}


PacketCountFilterCfg::PacketCountFilterCfg(XMLElement *e)
	: Cfg(e), instance(NULL)
{
}

PacketCountFilterCfg::~PacketCountFilterCfg()
{

}

Module* PacketCountFilterCfg::getInstance()
{
	if (!instance)
		instance = new SystematicSampler(SYSTEMATIC_SAMPLER_COUNT_BASED,
						getInterval(), getSpacing());

	return (Module*)instance;
}



/** helper function to return the real value of the string (HEX or normal) */
static std::string getRealValue(XMLElement* e)
{
	std::string str = e->getFirstText();
	XMLAttribute* a = e->getAttribute("type");

	if (a && a->getValue() == "HEX")
		return stringFilter::hexparser(str);

	return str;
}


Module* PacketStringFilterCfg::getInstance()
{
	if (!instance)
		instance = new stringFilter();

	XMLNode::XMLSet<XMLElement*> set = _elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("is")) {
			instance->addandFilter(getRealValue(e));
		} else if (e->matches("isnot")) {
			instance->addnotFilter(getRealValue(e));
		} else {
			msg(MSG_FATAL, "Unkown string packet filter config %s\n", e->getName().c_str());
			continue;
		}
	}

	return (Module*)instance;
}

bool PacketStringFilterCfg::deriveFrom(PacketStringFilterCfg* old)
{
	XMLNode::XMLSet<XMLElement*> newStatements = this->_elem->getElementChildren();
	XMLNode::XMLSet<XMLElement*> oldStatements = old->_elem->getElementChildren();

	if (newStatements.size() != oldStatements.size())
		return false;

	XMLNode::XMLSet<XMLElement*>::iterator itNew = newStatements.begin();
	XMLNode::XMLSet<XMLElement*>::iterator itOld = oldStatements.begin();
	for (; itNew != newStatements.end() && itOld != oldStatements.end();
	     itOld++ , itNew++) {
		XMLElement* oldE = *itOld;
		XMLElement* newE = *itNew;
		if (oldE->getFirstText() != newE->getFirstText())
			return false;
	}

	return true;
}

// ----------------------------------------------------------------------------

Module* PacketRegexFilterCfg::getInstance()
{
	if (!instance)
		instance = new regExFilter();

	instance->match = get("matchPattern");
	instance->regcre();
	return (Module*)instance;
}

bool PacketRegexFilterCfg::deriveFrom(PacketRegexFilterCfg* old)
{
	if (get("matchPattern") == old->get("matchPattern"))
		return true;

	return false;
}
