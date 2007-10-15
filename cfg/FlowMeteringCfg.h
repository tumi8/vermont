#ifndef FLOWMETERINGCFG_H_
#define FLOWMETERINGCFG_H_

#include "Cfg.h"
#include "InfoElementCfg.h"
#include "concentrator/IpfixAggregator.hpp"
//#include "concentrator/Rule.hpp"

class Rule;
class Rules;

class FlowMeteringCfg
	: public CfgHelper<IpfixAggregator, FlowMeteringCfg>
{
public:
	friend class ConfigManager;
	
	virtual ~FlowMeteringCfg();

	virtual FlowMeteringCfg* create(XMLElement* elem);
	
	virtual std::string getName() { return "flowMetering"; }
	
	virtual IpfixAggregator* createInstance();
	
	bool deriveFrom(FlowMeteringCfg* old);

	Rule* readRule(XMLElement* elem);
protected:
	FlowMeteringCfg(XMLElement* elem);
	
private:
	Rule::Field* readFlowKeyRule(XMLElement* e);
	Rule::Field* readNonFlowKeyRule(XMLElement* e);
	
	unsigned maxBufferTime;	
	unsigned minBufferTime;

	Rules* rules;
};

#endif /*FLOWMETERINGCFG_H_*/
