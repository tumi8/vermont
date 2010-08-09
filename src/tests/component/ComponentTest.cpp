#include <unistd.h>
#include <fstream>
#include <stdlib.h>
#include <getopt.h>

#include "ComponentTest.h"
#include "QueueTest.h"
#include "ConcurrentQueueTest.h"

using namespace std;

static void usage();

int main(int argc, char* argv[])
{
	uint32_t replication = 1;
	char* outputFile = (char*)"componentTestOut.txt";
	uint32_t component = 1;

	//Queue
	uint32_t queueType = 0;
	uint32_t numQueueProducers = 1;
	uint32_t numQueueConsumers = 1;
	uint32_t queueSize = 1000;
	uint32_t numQueueOps = 1000000;
	uint32_t timeoutLength = 100;

	//get command line parameter
	while (1) {
		uint32_t c;
		static struct option long_options[] =
		{
			{"output", 1, 0, '0'},
			{"rep", 1, 0, '1'},
			{"comp", 1, 0, '2'},
			{"qtype", 1, 0, '3'},
			{"qprod", 1, 0, '4'},
			{"qcons", 1, 0, '5'},
			{"qsize", 1, 0, '6'},
			{"qops", 1, 0, '7'},
			{"timeout", 1, 0, '8'},
			{"help", 1, 0, '9'},
			{0, 0, 0, 0}
		};

		c = getopt_long(argc, argv, "0:1:2:3:4:5:6:7:8:9", long_options, NULL);
		if (c == -1)
			break;

		switch (c) {
			case '0':
				outputFile=optarg;
				break;
			case '1':
				replication=atoi(optarg);
				break;
			case '2':
				component=atoi(optarg);
				break;
			case '3':
				queueType=atoi(optarg);
				break;
			case '4':
				numQueueProducers=atoi(optarg);
				break;
			case '5':
				numQueueConsumers=atoi(optarg);
				break;
			case '6':
				queueSize=atoi(optarg);
				break;
			case '7':
				numQueueOps=atoi(optarg);
				break;
			case '8':
				timeoutLength=atoi(optarg);
				break;
			case '9':
			default:
				usage();
				exit(1);
		}
	}

	//Queue implementation tests
	if(component == 1){
		returnClass res;
		QueueTest test(queueType, numQueueProducers, numQueueConsumers, queueSize);

		ofstream outf(outputFile);
		outf << "Tested component: queue implementation with";
		outf <<	" type:" << queueType;
		outf << " producers:" << numQueueProducers;
		outf << " consumers:" << numQueueConsumers;
		outf << " size:" << queueSize;
		outf << " operations:" << numQueueOps;
		outf << " timeout:" << timeoutLength << endl;
		outf.close();

		for(uint32_t i=0; i<replication; i++){
			res = test.runTest(numQueueOps);

			//write to outfile
			ofstream outf(outputFile, ios::app);
			outf << res.timespent.tv_sec << ".";
			outf.width(9);
			outf.fill('0');
			outf << res.timespent.tv_nsec;
			outf << " " << res.full << " " << res.empty << endl;
			outf.close();
		}
	}

	//ConcurrentQueue tests
	if(component == 2){
		returnClassCon res;
		ConcurrentQueueTest test(queueType, numQueueProducers, numQueueConsumers, queueSize, timeoutLength);

		ofstream outf(outputFile);
		outf << "Tested component: ConcurrentQueue with";
		outf <<	" type:" << queueType;
		outf << " producers:" << numQueueProducers;
		outf << " consumers:" << numQueueConsumers;
		outf << " size:" << queueSize;
		outf << " operations:" << numQueueOps;
		outf << " timeout:" << timeoutLength << endl;
		outf.close();

		for(uint32_t i=0; i<replication; i++){
			res = test.runTest(numQueueOps);

			//write to outfile
			ofstream outf(outputFile, ios::app);
			outf << res.timespent.tv_sec << ".";
			outf.width(9);
			outf.fill('0');
			outf << res.timespent.tv_nsec;
			outf << " " << res.full << " " << res.empty << endl;
			outf.close();
		}
	}

	//wrong component number
	if(component > 2 || component < 1){
		printf("Wrong component number\n\n");
		usage();
	}

	return 0;
}

static void usage()
{
	printf(
		"Vermont ComponentTest\n" \
		"    --output <file>  name output file\n" \
		"    --rep <num>      number of replications\n" \
		"    --comp <num>     component to test\n" \
		"    --qtype <num>    type of component\n" \
		"    --qprod <num>    number of producer threads\n" \
		"    --qcons <num>    number of consumer threads\n" \
		"    --qsize <num>    size of component\n" \
		"    --qops <num>     test component num times\n" \
		"    --timeout <num>  timeout for empty/full queue\n" \
		"    --help           print this helptext\n"
	);
}
