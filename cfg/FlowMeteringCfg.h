#ifndef FLOWMETERINGCFG_H_
#define FLOWMETERINGCFG_H_

#include "Cfg.h"
#include "InfoElementCfg.h"
#include "concentrator/IpfixAggregator.hpp"
//#include "concentrator/Rule.hpp"

class Rule;
class Rules;

class FlowMeteringCfg
	: public Cfg, CfgHelper<IpfixAggregator>
{
public:
	friend class ConfigManager;
	
	virtual ~FlowMeteringCfg();

	virtual FlowMeteringCfg* create(XMLElement* elem);
	
	virtual std::string getName() { return "flowMetering"; }
	
	virtual IpfixAggregator* getInstance();
	
	bool deriveFrom(Cfg* old);
		
	bool deriveFrom(FlowMeteringCfg* old);

	virtual void connectInstances(Cfg* other)
	{
		instance = getInstance();

		int need_adapter = 0;
		need_adapter |= ((getNext().size() > 1) ? NEED_SPLITTER : NO_ADAPTER);

		if ((dynamic_cast<Notifiable*>(other->getInstance()) != NULL) &&
		    (dynamic_cast<Timer*>(instance) == NULL))
			need_adapter |= NEED_TIMEOUT;
		
		connectTo(other->getInstance(), need_adapter);
	}

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
