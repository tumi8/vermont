#include "PacketAggregatorCfg.h"

PacketAggregatorCfg::PacketAggregatorCfg(XMLElement* elem)
	: CfgHelper<PacketAggregator, PacketAggregatorCfg>(elem, "packetAggregator"),
	AggregatorBaseCfg(elem)
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
	instance = new PacketAggregator(pollInterval);
	instance->buildAggregator(rules, minBufferTime, maxBufferTime, htableBits);

	return instance;
}

bool PacketAggregatorCfg::deriveFrom(PacketAggregatorCfg* old)
{
	return false;  // FIXME: implement it, to gain performance increase in reconnect
}
