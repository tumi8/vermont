#include <unistd.h>
#include <fstream>
#include <stdlib.h>
#include <getopt.h>

#include "ComponentTest.h"
#include "QueueTest.h"

using namespace std;

static void usage();

int main(int argc, char* argv[])
{
	uint32_t replication = 1;
	char* outputFile = (char*)"componentTestOut.txt";

	//Queue
	uint32_t queueType = 0;
	uint32_t numQueueProducers = 1;
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
			{"qtype", 1, 0, '2'},
			{"qprod", 1, 0, '3'},
			{"qsize", 1, 0, '4'},
			{"qops", 1, 0, '5'},
			{"timeout", 1, 0, '6'},
			{"help", 1, 0, '7'},
			{0, 0, 0, 0}
		};

		c = getopt_long(argc, argv, "0:1:2:3:4:5:6:7:", long_options, NULL);
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
				queueType=atoi(optarg);
				break;
			case '3':
				numQueueProducers=atoi(optarg);
				break;
			case '4':
				queueSize=atoi(optarg);
				break;
			case '5':
				numQueueOps=atoi(optarg);
				break;
			case '6':
				timeoutLength=atoi(optarg);
				break;
			case '7':
			default:
				usage();
				exit(1);
		}
	}

	//ConcurrentQueue tests
	if(queueType > 0){
		struct timespec res;
		QueueTest test(queueType, numQueueProducers, queueSize, timeoutLength);

		ofstream outf(outputFile);
		outf << "Tested ConcurrentQueue type:" << queueType;
		outf << " producers:" << numQueueProducers;
		outf << " size:" << queueSize;
		outf << " operations:" << numQueueOps;
		outf << " timeout:" << timeoutLength << endl;
		outf.close();

		for(uint32_t i=0; i<replication; i++){
			res = test.runTest(numQueueOps);

			//write to outfile
			ofstream outf(outputFile, ios::app);
			outf << res.tv_sec << ".";
			outf.width(9);
			outf.fill('0');
			outf << res.tv_nsec;
			outf << " sec" << endl;
			outf.close();
		}
	}

	return 0;
}

static void usage()
{
	printf(
		"Vermont ComponentTest\n" \
		"    --output <file>  name output file\n" \
		"    --rep <num>      number of replications\n" \
		"    --qtype <num>    type of ConcurrentQueue\n" \
		"    --qprod <num>    number of producer threads\n" \
		"    --qsize <num>    size of ConcurrentQueue\n" \
		"    --qops <num>     test ConcurrentQueue num times\n" \
		"    --help           print this helptext\n"
	);
}
