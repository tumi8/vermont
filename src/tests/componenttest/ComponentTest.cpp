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
	int replication = 1;
	char* outputFile = (char*)"componentTestOut.txt";

	//Queue
	int queueType = 0;
	int numQueueProducers = 1;
	int queueSize = 1000;
	int numQueueOps = 1000000;

	//get command line parameter
	while (1) {
		int c;
		static struct option long_options[] =
		{
			{"qprod", 1, 0, '0'},
			{"qsize", 1, 0, '1'},
			{"qops", 1, 0, '2'},
			{0, 0, 0, 0}
		};

		c = getopt_long(argc, argv, "ho:r:q:0:1:2:", long_options, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'o':
				outputFile=optarg;
				break;
			case 'r':
				replication=atoi(optarg);
				break;
			case 'q':
				queueType=atoi(optarg);
				break;
			case '0':
				numQueueProducers=atoi(optarg);
				break;
			case '1':
				queueSize=atoi(optarg);
				break;
			case '2':
				numQueueOps=atoi(optarg);
				break;
			case 'h':
			default:
				usage();
				exit(1);
		}
	}

	//ConcurrentQueue tests
	if(queueType > 0){
		struct timespec res;
		QueueTest test(queueType, numQueueProducers, queueSize);

		ofstream outf(outputFile);
		outf << "Tested ConcurrentQueue type:" << queueType;
		outf << " producers:" << numQueueProducers;
		outf << " size:" << queueSize;
		outf << " operations:" << numQueueOps << endl;
		outf.close();

		for(int i=0; i<replication; i++){
			res = test.runTest(numQueueOps);

			//write to outfile
			ofstream outf(outputFile, ios::app);
			outf << res.tv_sec << " sec " << res.tv_nsec << " nsec" << endl;
			outf.close();
		}
	}

	return 0;
}

static void usage()
{
	printf(
		"Vermont ComponentTest\n" \
		"    -o              name output file\n" \
		"    -q <num>        type of ConcurrentQueue. Options:[1|2|3]\n" \
		"    --qprod <num>   number of producer threads\n" \
		"    --qsize <num>   size of ConcurrentQueue\n" \
		"    --qops <num>    test ConcurrentQueue num times\n" \
		"    -r              number of replications\n" \
		"    -h              print this helptext\n"
	);
}
