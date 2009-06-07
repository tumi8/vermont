#include "IpfixAggregatorCfg.h"
#include "Rule.hpp"
#include "Rules.hpp"

IpfixAggregatorCfg::IpfixAggregatorCfg(XMLElement* elem)
	: CfgHelper<IpfixAggregator, IpfixAggregatorCfg>(elem, "ipfixAggregator"),
	AggregatorBaseCfg(elem)
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
	instance = new IpfixAggregator(pollInterval);
	instance->buildAggregator(rules, minBufferTime, maxBufferTime, htableBits);

	return instance;
}

bool IpfixAggregatorCfg::deriveFrom(IpfixAggregatorCfg* old)
{
	return false;  // FIXME: implement it, to gain performance increase in reconnect
}

