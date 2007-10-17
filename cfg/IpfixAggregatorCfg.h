#ifndef FLOWMETERINGCFG_H_
#define FLOWMETERINGCFG_H_

#include "Cfg.h"
#include "InfoElementCfg.h"
#include "concentrator/IpfixAggregator.hpp"
//#include "concentrator/Rule.hpp"

class Rule;
class Rules;

class IpfixAggregatorCfg
	: public CfgHelper<IpfixAggregator, IpfixAggregatorCfg>
{
public:
	friend class ConfigManager;
	
	virtual ~IpfixAggregatorCfg();

	virtual IpfixAggregatorCfg* create(XMLElement* elem);
	
	virtual std::string getName() { return "flowMetering"; }
	
	virtual IpfixAggregator* createInstance();
	
	bool deriveFrom(IpfixAggregatorCfg* old);

	Rule* readRule(XMLElement* elem);
protected:
	IpfixAggregatorCfg(XMLElement* elem);
	
private:
	static Rule::Field* readFlowKeyRule(XMLElement* e);
	static Rule::Field* readNonFlowKeyRule(XMLElement* e);
	
	unsigned maxBufferTime;	
	unsigned minBufferTime;

	Rules* rules;
};

#endif /*FLOWMETERINGCFG_H_*/
