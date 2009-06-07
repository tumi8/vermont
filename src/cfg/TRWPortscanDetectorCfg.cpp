#include "TRWPortscanDetectorCfg.h"


TRWPortscanDetectorCfg* TRWPortscanDetectorCfg::create(XMLElement* e)
{
    assert(e);
    assert(e->getName() == getName());
    return new TRWPortscanDetectorCfg(e);
}

TRWPortscanDetectorCfg::TRWPortscanDetectorCfg(XMLElement* elem)
    : CfgHelper<TRWPortscanDetector, TRWPortscanDetectorCfg>(elem, "trwPortscanDetector"),
      hashBits(20),
      timeExpirePending(60*60*24),
      timeExpireScanner(60*30),
      timeExpireBenign(60*30),
      timeCleanupInterval(10)
{
    if (!elem) return;

    XMLNode::XMLSet<XMLElement*> set = _elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("hashbits")) {
			hashBits = getInt("hashbits");
		} else if (e->matches("timeexpirepending")) {
			timeExpirePending = getInt("timeexpirepending");
		} else if (e->matches("timeexpirescanner")) {
			timeExpireScanner = getInt("timeexpirescanner");
		} else if (e->matches("timeexpirebenign")) {
			timeExpireBenign = getInt("timeexpirebenign");
		} else if (e->matches("timecleanupinterval")) {
			timeCleanupInterval = getInt("timecleanupinterval");
		} else if (e->matches("analyzerid")) {
			analyzerId = e->getFirstText();
		} else if (e->matches("idmeftemplate")) {
			idmefTemplate = e->getFirstText();
		} else if (e->matches("next")) { // ignore next
		} else {
			msg(MSG_FATAL, "Unknown TRWPortscanDetector config statement %s\n", e->getName().c_str());
			continue;
		}
	}
	if (analyzerId=="") THROWEXCEPTION("TRWPortscanDetectorCfg: analyzerid not set in configuration!");
	if (idmefTemplate=="") THROWEXCEPTION("TRWPortscanDetectorCfg: idmeftemplate not set in configuration!");
}

TRWPortscanDetectorCfg::~TRWPortscanDetectorCfg()
{
}

TRWPortscanDetector* TRWPortscanDetectorCfg::createInstance()
{
    instance = new TRWPortscanDetector(hashBits, timeExpirePending, timeExpireScanner, timeExpireBenign, timeCleanupInterval, analyzerId, idmefTemplate);
    return instance;
}

bool TRWPortscanDetectorCfg::deriveFrom(TRWPortscanDetectorCfg* old)
{
    return false;
}
