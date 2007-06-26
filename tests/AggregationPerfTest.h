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
		AggregationPerfTest();
		~AggregationPerfTest();

		void setup();
		void start(unsigned int numpackets);
		void shutdown();

	private:
		Filter* filter;
		PacketSink* packetSink;
		HookingFilter* hookingFilter;
		IpfixAggregator* ipfixAggregator;
		InstanceManager<Packet>* packetManager;

		Rule::Field* createRuleField(const string& typeId);
		Rules* createRules();
};

#endif
