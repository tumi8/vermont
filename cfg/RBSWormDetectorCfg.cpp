#include "RBSWormDetectorCfg.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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
			lambda_ratio = getDouble("lambdaratio"); 
		} else if (e->matches("subnet")) {
			string ipstring = e->getFirstText();
			uint32_t ip = 0;
			uint32_t subnet = inet_addr("255.255.255.255");
			int last_pos = ipstring.find("/");
		        if (last_pos)
			{
		        string tempstr = ipstring.substr(0,last_pos);
		        ip = inet_addr(tempstr.c_str());
			tempstr = ipstring.substr(last_pos+1,ipstring.length());
			subnet <<= (32-atoi(tempstr.c_str()));
			}
			subnets[ip] = htonl(subnet);
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
    instance = new RBSWormDetector(hashBits, timeExpirePending, timeExpireWorm, timeExpireBenign, timeAdaptInterval, timeCleanupInterval,lambda_ratio, analyzerId, idmefTemplate, subnets);
    return instance;
}

bool RBSWormDetectorCfg::deriveFrom(RBSWormDetectorCfg* old)
{
    return false;
}
