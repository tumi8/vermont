#include "PCAPExporterCfg.h"

#include "common/defs.h"

#include <cassert>
#include <pcap.h>

PCAPExporterCfg::PCAPExporterCfg(XMLElement* elem) 
	: CfgHelper<PCAPExporterModule, PCAPExporterCfg>(elem, "pcapExporter"), link_type(DLT_EN10MB)
{ 
	if (!elem) return;

	XMLNode::XMLSet<XMLElement*> set = elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("filename")) {
			fileName = e->getFirstText();
		} else if (e->matches("linkType")) {
			link_type = pcap_datalink_name_to_val(e->getFirstText().c_str());
		}
	}
} 

PCAPExporterCfg* PCAPExporterCfg::create(XMLElement* elem)
{
	assert(elem);
	assert(elem->getName() == getName());
	return new PCAPExporterCfg(elem);
}

PCAPExporterCfg::~PCAPExporterCfg()
{
}

PCAPExporterModule* PCAPExporterCfg::createInstance()
{
	instance = new PCAPExporterModule(fileName);
	return instance;
}

bool PCAPExporterCfg::deriveFrom(PCAPExporterCfg* old)
{
	return false; // FIXME: implement
}
