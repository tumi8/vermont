#include "VermontTest.h"
#include "AggregationPerfTest.h"
#include "ReconfTest.h"
#include "test_concentrator.h"




int main(int argc, char* argv[])
{
	printf("Vermont Testsuite, testing ...\n");

	bool perftest = false;

	if (argc>1 && strcmp(argv[1], "-perf")==0) perftest = true;

	//msg_setlevel(MSG_DEBUG);
	
	ReconfTest reconf;
	reconf.normalTest();
	
	AggregationPerfTest aggtest(!perftest);
	aggtest.execute();
	
	ConcentratorTestSuite conctest;
	conctest.start_test();
	
	return 0;
}
