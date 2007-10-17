#include "IpfixAggregatorCfg.h"
#include "concentrator/Rule.hpp"
#include "concentrator/Rules.hpp"

IpfixAggregatorCfg::IpfixAggregatorCfg(XMLElement* elem)
	: CfgHelper<IpfixAggregator, IpfixAggregatorCfg>(elem), AggregatorBaseCfg(elem)
{
}

IpfixAggregatorCfg::~IpfixAggregatorCfg()
{
	if (instance == NULL)
		delete rules;
}

IpfixAggregatorCfg* IpfixAggregatorCfg::create(XMLElement* elem)
{
	assert(elem);
	assert(elem->getName() == getName());
	return new IpfixAggregatorCfg(elem);
}

IpfixAggregator* IpfixAggregatorCfg::createInstance()
{
	instance = new IpfixAggregator(0); // FIXME: where to get the parameter pollinterval?
	instance->buildAggregator(rules, minBufferTime, maxBufferTime);

	return instance;
}

bool IpfixAggregatorCfg::deriveFrom(IpfixAggregatorCfg* old)
{
	return false;  // FIXME: implement it, to gain performance increase in reconnect
}

