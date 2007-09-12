#if !defined(AGGREGATIONPERFTEST_H)
#define AGGREGATIONPERFTEST_H

#include "sampler/Filter.h"
#include "sampler/PacketSink.h"
#include "sampler/HookingFilter.h"
#include "concentrator/IpfixAggregator.hpp"
#include "common/InstanceManager.h"



class AggregationPerfTest
{
	public:
		AggregationPerfTest(bool fast);
		~AggregationPerfTest();

		void normalTest();
		void expressTest();

	private:
		Filter* filter;
		PacketSink* packetSink;
		PacketProcessor* hookingFilter;
		IpfixAggregator* ipfixAggregator;
		InstanceManager<Packet>* packetManager;

		Rule::Field* createRuleField(const string& typeId);
		Rules* createRules();
		void setup(bool express);
		void start(unsigned int numpackets);
		void shutdown();

		int numPackets;
};

#endif
