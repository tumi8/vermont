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
			int tmp =  pcap_datalink_name_to_val(e->getFirstText().c_str());
			if (tmp == -1) {
				msg(MSG_ERROR, "Found illegal link type");
			} else {
				link_type = tmp;
			}
		} else if (e->matches("snaplen")) {
			snaplen = getInt("snaplen", PCAP_MAX_CAPTURE_LENGTH, e);
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
	instance->setDataLinkType(link_type);
	instance->setSnaplen(snaplen);
	return instance;
}

bool PCAPExporterCfg::deriveFrom(PCAPExporterCfg* old)
{
	return false; // FIXME: implement
}
