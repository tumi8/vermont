#if !defined(AGGREGATIONPERFTEST_H)
#define AGGREGATIONPERFTEST_H

#include <boost/test/unit_test.hpp>

#include "sampler/Filter.h"
#include "sampler/PacketSink.h"
#include "sampler/HookingFilter.h"
#include "concentrator/IpfixAggregator.hpp"
#include "common/InstanceManager.h"

using boost::unit_test::test_suite;
using boost::shared_ptr;
using boost::unit_test::test_case;

class AggregationPerfTest
{
	public:
		AggregationPerfTest(bool fast);
		~AggregationPerfTest();

		void setup(bool express);
		void start(unsigned int numpackets);
		void shutdown();
		void normalTest();
		void expressTest();

	private:
		Filter* filter;
		PacketSink* packetSink;
		PacketProcessor* hookingFilter;
		IpfixAggregator* ipfixAggregator;
		InstanceManager<Packet>* packetManager;

		Rule::Field* createRuleField(const std::string& typeId);
		Rules* createRules();

		int numPackets;
};

class AggregationPerfTestSuite : public test_suite
{
	public:
		AggregationPerfTestSuite(bool fast)
			: test_suite("AggregationPerfTest")
		{
			shared_ptr<AggregationPerfTest> inst(new AggregationPerfTest(fast));
			test_case* normaltest = BOOST_CLASS_TEST_CASE(&AggregationPerfTest::normalTest, inst);
			test_case* expresstest = BOOST_CLASS_TEST_CASE(&AggregationPerfTest::expressTest, inst);

			//deposit_test_case->depends_on( init_test_case );
			//withdraw_test_case->depends_on( deposit_test_case );

			add(normaltest);
			add(expresstest);
		}
};
#endif
