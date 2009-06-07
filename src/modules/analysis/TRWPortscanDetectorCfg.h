#ifndef TRWPORTSCANDETECTORCFG_H_
#define TRWPORTSCANDETECTORCFG_H_

#include <core/XMLElement.h>
#include <core/Cfg.h>

#include "modules/analysis/TRWPortscanDetector.h"

#include <string>

using namespace std;


class TRWPortscanDetectorCfg
	: public CfgHelper<TRWPortscanDetector, TRWPortscanDetectorCfg>
{
public:
	friend class ConfigManager;
	
	virtual TRWPortscanDetectorCfg* create(XMLElement* e);
	virtual ~TRWPortscanDetectorCfg();
	
	virtual TRWPortscanDetector* createInstance();
	virtual bool deriveFrom(TRWPortscanDetectorCfg* old);
	
protected:
	
	uint32_t hashBits;	/**< amount of bits used for hashtable */
	uint32_t timeExpirePending; // time in seconds until pending entries are expired
	uint32_t timeExpireScanner; // time in seconds until scanner entries are expired
	uint32_t timeExpireBenign; // time in seconds until benign entries are expired
	uint32_t timeCleanupInterval; // time in seconds of interval when hashtable with source hosts is cleaned up (trwEntries)
	string analyzerId;	/**< analyzer id for IDMEF messages */
	string idmefTemplate;	/**< template file for IDMEF messages */
	
	TRWPortscanDetectorCfg(XMLElement*);
};


#endif /*TRWPORTSCANDETECTORCFG_H_*/
