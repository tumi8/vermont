#include "common/msg.h"
#include "core/XMLElement.h"

#include "IpfixFileWriterCfg.hpp"

#include <string>
#include <vector>
#include <cassert>

IpfixFileWriterCfg* IpfixFileWriterCfg::create(XMLElement* e)
{
	assert(e);
	assert(e->getName() == getName());
	return new IpfixFileWriterCfg(e);
}

IpfixFileWriterCfg::IpfixFileWriterCfg(XMLElement* elem)
	: CfgHelper<IpfixFileWriter, IpfixFileWriterCfg>(elem, "ipfixFileWriter"),
	destinationPath("./"),
	filenamePrefix("ipfix.dump"),
	maximumFilesize(DEFAULTFILESIZE),
	observationDomainId(0)
{
	if (!elem) return;  // needed because of table inside ConfigManager

	XMLNode::XMLSet<XMLElement*> set = _elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("maximumFilesize")) {
			maximumFilesize = getInt("maximumFilesize"); 
		}else if (e->matches("destinationPath")){
			destinationPath = e->getFirstText();
		}else if (e->matches("filenamePrefix")){
			filenamePrefix = e->getFirstText();
		} else if (e->matches("observationDomainId")) {
			observationDomainId = getInt("observationDomainId");
		}
		 else {
			msg(MSG_FATAL, "Unknown ipfixFileWriter config statement %s\n", e->getName().c_str());
			continue;
		}
	}
}

IpfixFileWriterCfg::~IpfixFileWriterCfg()
{

}

IpfixFileWriter* IpfixFileWriterCfg::createInstance()
{
	instance = new IpfixFileWriter(observationDomainId, 
			filenamePrefix, destinationPath, maximumFilesize);
	return instance;
}

bool IpfixFileWriterCfg::deriveFrom(IpfixFileWriterCfg* old)
{
	if (maximumFilesize != old->maximumFilesize ||
	    destinationPath != old->destinationPath ||
	    filenamePrefix != old->filenamePrefix
	    ) return false;
		
	return true;
}

