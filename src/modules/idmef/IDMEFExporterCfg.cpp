#include "IDMEFExporterCfg.h"

#include "common/msg.h"

IDMEFExporterCfg* IDMEFExporterCfg::create(XMLElement* e)
{
    ASSERT(e, "e must not be NULL");
    ASSERT(e->getName() == getName(), "invalid name specified for this module");
    return new IDMEFExporterCfg(e);
}

IDMEFExporterCfg::IDMEFExporterCfg(XMLElement* elem)
    : CfgHelper<IDMEFExporter, IDMEFExporterCfg>(elem, "idmefExporter"),
      destDirectory("idmef_work")
{
    if (!elem) return;

    XMLNode::XMLSet<XMLElement*> set = _elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("destdir")) {
			destDirectory = e->getFirstText();
		} else if (e->matches("sendurl")) {
			sendURL = e->getFirstText();
		} else {
			msg(MSG_FATAL, "Unknown IDMEFExporter config statement %s\n", e->getName().c_str());
			continue;
		}
	}
	
	if (sendURL == "") THROWEXCEPTION("no destination URL specified for IDMEFExporter");
}

IDMEFExporterCfg::~IDMEFExporterCfg()
{
}

IDMEFExporter* IDMEFExporterCfg::createInstance()
{
    instance = new IDMEFExporter(destDirectory, sendURL);
    return instance;
}

bool IDMEFExporterCfg::deriveFrom(IDMEFExporterCfg* old)
{
    return false;
}
