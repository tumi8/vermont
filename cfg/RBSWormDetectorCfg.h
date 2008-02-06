#ifndef RBSWORMDETECTORCFG_H_
#define RBSWORMDETECTORCFG_H_

#include <cfg/XMLElement.h>
#include <cfg/Cfg.h>

#include "concentrator/RBSWormDetector.h"

#include <string>

using namespace std;


class RBSWormDetectorCfg
	: public CfgHelper<RBSWormDetector, RBSWormDetectorCfg>
{
public:
	friend class ConfigManager;
	
	virtual RBSWormDetectorCfg* create(XMLElement* e);
	virtual ~RBSWormDetectorCfg();
	
	virtual RBSWormDetector* createInstance();
	virtual bool deriveFrom(RBSWormDetectorCfg* old);
	
protected:
	
	uint32_t hashBits;	/**< amount of bits used for hashtable */
	uint32_t timeExpirePending; // time in seconds until pending entries are expired
	uint32_t timeExpireWorm; // time in seconds until worm entries are expired
	uint32_t timeExpireBenign; // time in seconds until benign entries are expired
	uint32_t timeAdaptInterval; // time in seconds of interval when lambdas are being recalculated
	uint32_t timeCleanupInterval; // time in seconds of interval when hashtable with source hosts is cleaned up (trwEntries)
	string analyzerId;	/**< analyzer id for IDMEF messages */
	string idmefTemplate;	/**< template file for IDMEF messages */
	float lambda_ratio; //**< expected connection ratio of good and bad hosts */
	
	RBSWormDetectorCfg(XMLElement*);
};


#endif /*RBSWORMDETECTORCFG_H_*/
