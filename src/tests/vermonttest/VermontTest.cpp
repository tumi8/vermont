#include "VermontTest.h"
#include "AggregationPerfTest.h"
#include "ReconfTest.h"
#include "BloomFilterTest.h" 
#include "ConnectionFilterTest.h"
#include "test_concentrator.h"
#include "ConfigTester.h"
#include "StructuredDataTest.h"

#include "TestSuiteBase.h"



int main(int argc, char* argv[])
{
	printf("Vermont Testsuite, testing ...\n");

	bool perftest = false;

	if (argc>1 && strcmp(argv[1], "-perf")==0) perftest = true;

	// msg_setlevel(MSG_DEBUG);
	
	StructuredDataTestSuite structuredDataTestSuite;
	structuredDataTestSuite.run();

	TestSuite testSuite;

	testSuite.add(new ReconfTest());
	testSuite.add(new AggregationPerfTest(!perftest));
	testSuite.add(new ConcentratorTestSuite());
#ifdef HAVE_CONNECTION_FILTER
	testSuite.add(new BloomFilterTestSuite());
	testSuite.add(new ConnectionFilterTestSuite());
#endif
	testSuite.add(new ConfigTester());

	testSuite.run();

	return 0;
}

