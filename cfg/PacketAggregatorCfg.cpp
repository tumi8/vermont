#include "PacketAggregatorCfg.h"

PacketAggregatorCfg::PacketAggregatorCfg(XMLElement* elem)
	: CfgHelper<PacketAggregator, PacketAggregatorCfg>(elem), AggregatorBaseCfg(elem)
{
}

PacketAggregatorCfg::~PacketAggregatorCfg()
{
	if (instance == NULL)
		delete rules;
}

PacketAggregatorCfg* PacketAggregatorCfg::create(XMLElement* elem)
{
	assert(elem);
	assert(elem->getName() == getName());
	return new PacketAggregatorCfg(elem);
}

PacketAggregator* PacketAggregatorCfg::createInstance()
{
	instance = new PacketAggregator(0); // FIXME: where to get the parameter pollinterval?
	instance->buildAggregator(rules, minBufferTime, maxBufferTime);

	return instance;
}

bool PacketAggregatorCfg::deriveFrom(PacketAggregatorCfg* old)
{
	return false;  // FIXME: implement it, to gain performance increase in reconnect
}
