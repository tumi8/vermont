#include <getopt.h>

#include "VermontTest.h"
#include "AggregationPerfTest.h"
#include "ReconfTest.h"
#include "BloomFilterTest.h" 
#include "ConnectionFilterTest.h"
#include "test_concentrator.h"
#include "ConfigTester.h"

#include "TestSuiteBase.h"



int main(int argc, char* argv[])
{
	printf("Vermont Testsuite, testing ...\n");

	bool perftest = false;
	int opt, option_index;
	const char *config_dir = nullptr;

	static const struct option long_opts[] = {
		{ "config-dir",  required_argument, NULL, 'c' },
		{ "perf",     required_argument, NULL, 'p' },
		{ NULL, 0, NULL, 0}
	};

	while ((opt = getopt_long(argc, argv, "pc:", long_opts,
		&option_index)) != EOF) {
		switch (opt) {
		case 'p':
			perftest = true;
			break;
		case 'c':
			config_dir = optarg;
			break;
		}
	}

	if (!config_dir) {
		ERROR("--config-dir is required");
	}

	//msg_setlevel(LOG_INFO);
	
	TestSuite testSuite;

	testSuite.add(new ReconfTest());
	testSuite.add(new AggregationPerfTest(!perftest));
	testSuite.add(new ConcentratorTestSuite());
#ifdef HAVE_CONNECTION_FILTER
	testSuite.add(new BloomFilterTestSuite());
	testSuite.add(new ConnectionFilterTestSuite());
#endif
	testSuite.add(new ConfigTester(config_dir));

	testSuite.run();

	return 0;
}

