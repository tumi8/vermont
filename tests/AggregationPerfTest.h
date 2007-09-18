#if !defined(AGGREGATIONPERFTEST_H)
#define AGGREGATIONPERFTEST_H

#include "sampler/Filter.h"
#include "sampler/PacketSink.h"
#include "concentrator/IpfixAggregator.hpp"
#include "common/InstanceManager.h"



class AggregationPerfTest
{
	public:
		AggregationPerfTest(bool fast);
		~AggregationPerfTest();
		
		void execute();


	private:
		InstanceManager<Packet>* packetManager;

		Rule::Field* createRuleField(const string& typeId);
		Rules* createRules();
		void sendPacketsTo(Destination<Packet*>* dest, uint32_t numpackets);

		int numPackets;
};

#endif
