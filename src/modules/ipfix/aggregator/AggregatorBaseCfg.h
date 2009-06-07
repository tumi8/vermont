#ifndef AGGREGATORBASECFG_H_
#define AGGREGATORBASECFG_H_

#include "core/Cfg.h"
#include "modules/ipfix/aggregator/Rule.hpp"

// forward declarations
class Rule;
class Rules;

class AggregatorBaseCfg
	: private CfgBase
{
public:
	AggregatorBaseCfg(XMLElement* elem);
	virtual ~AggregatorBaseCfg();

protected:
	Rule* readRule(XMLElement* elem);
	
	static Rule::Field* readFlowKeyRule(XMLElement* e);
	static Rule::Field* readNonFlowKeyRule(XMLElement* e);

	unsigned maxBufferTime;	
	unsigned minBufferTime;
	unsigned pollInterval;
	uint8_t htableBits;

	Rules* rules;
};

#endif /*AGGREGATORBASECFG_H_*/
