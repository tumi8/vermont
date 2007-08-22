#include "PacketFilterCfg.h"

#include "common/msg.h"
#include <sampler/SystematicSampler.h>

#include <cassert>


PacketFilterCfg::PacketFilterCfg(XMLElement* elem)
	: Cfg(elem), filter(NULL)
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
		} else if (e->matches("timeBased")) {
			msg(MSG_FATAL, "Filter: Creating time based sampler");
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

FilterModule* PacketFilterCfg::getInstance()
{
	if (filter != NULL)
		return filter;
	
	filter = new FilterModule();
	for (std::vector<Cfg*>::iterator it = subCfgs.begin();
	     it != subCfgs.end();
	     it++) {
		filter->addProcessor(reinterpret_cast<PacketProcessor*>((*it)->getInstance()));
	}
	return filter;
}

PacketCountFilterCfg::PacketCountFilterCfg(XMLElement *e)
	: Cfg(e)
{
}

PacketCountFilterCfg::~PacketCountFilterCfg()
{
}

Module* PacketCountFilterCfg::getInstance()
{
	SystematicSampler* s;
	s = new SystematicSampler(SYSTEMATIC_SAMPLER_COUNT_BASED,
			          getInterval(), getSpacing());

	return (Module*)s;
}
