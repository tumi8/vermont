#include "VermontTest.h"
#include "AggregationPerfTest.h"
#include "ReconfTest.h"
//#include "BloomFilterTest.h" tobi_merge
//#include "ConnectionFilterTest.h" tobi_merge
#include "test_concentrator.h"




int main(int argc, char* argv[])
{
	printf("Vermont Testsuite, testing ...\n");

	bool perftest = false;

	if (argc>1 && strcmp(argv[1], "-perf")==0) perftest = true;

	//msg_setlevel(MSG_DEBUG);

	ReconfTest reconf;
	reconf.normalTest();
	reconf.splitterTest();

	AggregationPerfTest aggtest(!perftest);
	aggtest.execute();

	ConcentratorTestSuite conctest;
	conctest.start_test();

	//test->add(new AggregationPerfTestSuite(!perftest));
	//test->add(new ConcentratorTestSuite());
#ifdef HAVE_CONNECTION_FILTER
	//test->add(new BloomFilterTestSuite()); tobi_merge
	//test->add(new ConnectionFilterTestSuite()); tobi_merge
#endif

	return 0;
}
