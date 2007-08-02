#define BOOST_AUTO_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include "VermontTest.h"
#include "AggregationPerfTest.h"
#include "test_concentrator.h"

using boost::unit_test::test_suite;



test_suite* init_unit_test_suite(int argc, char* argv[])
{
	test_suite* test(BOOST_TEST_SUITE("Vermont Testsuite"));

	bool perftest = false;

	if (argc>1 && strcmp(argv[1], "-perf")==0) perftest = true;

	test->add(new AggregationPerfTestSuite(!perftest));
	test->add(new ConcentratorTestSuite());

	return test;
}
