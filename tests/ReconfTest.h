#ifndef RECONFTEST_H_
#define RECONFTEST_H_

#include <boost/test/unit_test.hpp>

#include "reconf/ConnectionQueue.h"
#include "reconf/FilterModule.h"
#include "sampler/PacketSink.h"
#include "sampler/HookingFilter.h"
#include "concentrator/IpfixAggregator.hpp"
#include "common/InstanceManager.h"

using boost::unit_test::test_suite;
using boost::shared_ptr;
using boost::unit_test::test_case;

class ReconfTest
{
	public:
		ReconfTest();
		~ReconfTest();

		void setup(bool express);
		void sendPacketsTo(Destination<Packet*>* dest, size_t numpackets);
		void shutdown();
		void normalTest();
};

class ReconfTestSuite : public test_suite
{
	public:
		ReconfTestSuite()
			: test_suite("ReconfTest")
		{
			
			shared_ptr<ReconfTest> inst(new ReconfTest());
			test_case* normaltest = BOOST_CLASS_TEST_CASE(&ReconfTest::normalTest, inst);

			add(normaltest);
		}
};


#endif /*RECONFTEST_H_*/
