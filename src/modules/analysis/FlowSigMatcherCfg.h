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

	
	string analyzerId;	/**< analyzer id for IDMEF messages */
	string idmefTemplate;	/**< template file for IDMEF messages */
        string homenet; // <homenet i.e. 192.168.0.0/16
        string rulesfile; //Path and name of rulesfile
        string rulesorder; //Path and name of rulesfile
	string flagstimeout;

	FlowSigMatcherCfg(XMLElement*);
};


#endif /*FLOWSIGMATCHERCFG_H_*/
