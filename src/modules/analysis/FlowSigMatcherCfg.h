#ifndef FLOWSIGMATCHERCFG_H_
#define FLOWSIGMATCHERCFG_H_

#include <core/XMLElement.h>
#include <core/Cfg.h>

#include "modules/analysis/FlowSigMatcher.h"

#include <string>

using namespace std;


class FlowSigMatcherCfg
	: public CfgHelper<FlowSigMatcher, FlowSigMatcherCfg>
{
public:
	friend class ConfigManager;

	virtual FlowSigMatcherCfg* create(XMLElement* e);
	virtual ~FlowSigMatcherCfg();

	virtual FlowSigMatcher* createInstance();
	virtual bool deriveFrom(FlowSigMatcherCfg* old);

protected:

	uint32_t hashBits;	/**< amount of bits used for hashtable */
	uint32_t timeExpirePending; // time in seconds until pending entries are expired
	uint32_t timeExpireScanner; // time in seconds until scanner entries are expired
	uint32_t timeExpireBenign; // time in seconds until benign entries are expired
	uint32_t timeCleanupInterval; // time in seconds of interval when hashtable with source hosts is cleaned up (trwEntries)
	string analyzerId;	/**< analyzer id for IDMEF messages */
	string idmefTemplate;	/**< template file for IDMEF messages */

	FlowSigMatcherCfg(XMLElement*);
};


#endif /*FLOWSIGMATCHERCFG_H_*/
