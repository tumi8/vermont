#include "RBSWormDetectorCfg.h"


RBSWormDetectorCfg* RBSWormDetectorCfg::create(XMLElement* e)
{
    assert(e);
    assert(e->getName() == getName());
    return new RBSWormDetectorCfg(e);
}

RBSWormDetectorCfg::RBSWormDetectorCfg(XMLElement* elem)
    : CfgHelper<RBSWormDetector, RBSWormDetectorCfg>(elem, "rbsWormDetector"),
      hashBits(20),
      timeExpirePending(60*60*24),
      timeExpireWorm(60*30),
      timeExpireBenign(60*30),
	timeAdaptInterval(3600),
      timeCleanupInterval(100),
lambda_ratio(5)
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
		} else if (e->matches("timeexpireworm")) {
			timeExpireWorm = getInt("timeexpireworm");
		} else if (e->matches("timeexpirebenign")) {
			timeExpireBenign = getInt("timeexpirebenign");
		} else if (e->matches("timeadaptinterval")) {
			timeAdaptInterval = getInt("timeadaptinterval");
		} else if (e->matches("timecleanupinterval")) {
			timeCleanupInterval = getInt("timecleanupinterval");
		} else if (e->matches("lambdaratio")) {
			lambda_ratio = (float) getInt("lambdaratio"); //no getfloat :(
		} else if (e->matches("analyzerid")) {
			analyzerId = e->getFirstText();
		} else if (e->matches("idmeftemplate")) {
			idmefTemplate = e->getFirstText();
		} else if (e->matches("next")) { // ignore next
		} else {
			msg(MSG_FATAL, "Unknown RBSWormDetector config statement %s\n", e->getName().c_str());
			continue;
		}
	}
	if (analyzerId=="") THROWEXCEPTION("RBSWormDetectorCfg: analyzerid not set in configuration!");
	if (idmefTemplate=="") THROWEXCEPTION("RBSWormDetectorCfg: idmeftemplate not set in configuration!");
}

RBSWormDetectorCfg::~RBSWormDetectorCfg()
{
}

RBSWormDetector* RBSWormDetectorCfg::createInstance()
{
    instance = new RBSWormDetector(hashBits, timeExpirePending, timeExpireWorm, timeExpireBenign, timeAdaptInterval, timeCleanupInterval,lambda_ratio, analyzerId, idmefTemplate);
    return instance;
}

bool RBSWormDetectorCfg::deriveFrom(RBSWormDetectorCfg* old)
{
    return false;
}
