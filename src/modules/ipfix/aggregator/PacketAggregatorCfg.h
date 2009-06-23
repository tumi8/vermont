#ifndef PACKETAGGREGATORCFG_H_
#define PACKETAGGREGATORCFG_H_

#include "AggregatorBaseCfg.h"
#include "PacketAggregator.h"

class PacketAggregatorCfg
	: public CfgHelper<PacketAggregator, PacketAggregatorCfg>, public AggregatorBaseCfg
{
public:
	PacketAggregatorCfg(XMLElement* elem);
	virtual ~PacketAggregatorCfg();

	virtual PacketAggregatorCfg* create(XMLElement* elem);
	
	virtual PacketAggregator* createInstance();
	
	bool deriveFrom(PacketAggregatorCfg* old);
};

#endif /*PACKETAGGREGATORCFG_H_*/
