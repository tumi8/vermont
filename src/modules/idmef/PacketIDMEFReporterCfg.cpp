#include "PacketIDMEFReporterCfg.h"

#include "PacketIDMEFReporter.h"


PacketIDMEFReporterCfg* PacketIDMEFReporterCfg::create(XMLElement* e)
{
    ASSERT(e, "received invalid XML element");
    ASSERT(e->getName() == getName(), "received invalid XML element");
    return new PacketIDMEFReporterCfg(e);
}

PacketIDMEFReporterCfg::PacketIDMEFReporterCfg(XMLElement* elem)
    : CfgHelper<PacketIDMEFReporter, PacketIDMEFReporterCfg>(elem, "packetIdmefReporter"),
	  snapshotOffset(0), snapshotLength(0)
{
    if (!elem) return;

    XMLNode::XMLSet<XMLElement*> set = _elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("snapshotoffset")) {
			snapshotOffset = getInt("snapshotoffset");
		} else if (e->matches("snapshotlength")) {
			snapshotLength = getInt("snapshotlength");
		} else if (e->matches("analyzerid")) {
			analyzerId = e->getFirstText();
		} else if (e->matches("idmeftemplate")) {
			idmefTemplate = e->getFirstText();
		} else if (e->matches("next")) { // ignore next
		} else {
			msg(MSG_FATAL, "Unknown PacketIDMEFReporter config statement %s\n", e->getName().c_str());
			continue;
		}
	}
	if (analyzerId=="") THROWEXCEPTION("PacketIDMEFReporterCfg: analyzerid not set in configuration!");
	if (idmefTemplate=="") THROWEXCEPTION("PacketIDMEFReporterCfg: idmeftemplate not set in configuration!");
}

PacketIDMEFReporterCfg::~PacketIDMEFReporterCfg()
{
}

PacketIDMEFReporter* PacketIDMEFReporterCfg::createInstance()
{
    instance = new PacketIDMEFReporter(idmefTemplate, analyzerId, snapshotOffset, snapshotLength);
    return instance;
}

bool PacketIDMEFReporterCfg::deriveFrom(PacketIDMEFReporterCfg* old)
{
    return false;
}
