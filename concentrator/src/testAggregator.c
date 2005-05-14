/** \file
 * Separate Program to test the Aggregator module
 * Dumps received flows to stdout
 */

#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include "common.h"
#include "rcvIpfix.h"
#include "aggregator.h"
#include "sndIpfix.h"
#include "printIpfix.h"

int mayRun;

void sigint() {
	mayRun = 0;
	}

int main(int argc, char *argv[]) {
	mayRun = 1;
	signal(SIGINT, sigint);

	debug("initializing collectors");
	initializeIpfixReceivers();

	debug("initializing aggregators");
	initializeAggregators();
	
	debug("initializing printers");
	initializeIpfixPrinters();

	debug("initializing exporters");
	initializeIpfixSenders();

	debug("starting exporter");
	IpfixSender* ipfixSender = createIpfixSender(0xDEADBEEF, "127.0.0.1", 1501);
	startIpfixSender(ipfixSender);

	debug("starting printer");
	IpfixPrinter* ipfixPrinter = createIpfixPrinter();
	startIpfixPrinter(ipfixPrinter);
		
	debug("starting aggregator");
	IpfixAggregator* ipfixAggregator = createAggregator("aggregation_rules.conf", 5, 15);
	addAggregatorCallbacks(ipfixAggregator, getIpfixSenderCallbackInfo(ipfixSender));
	addAggregatorCallbacks(ipfixAggregator, getIpfixPrinterCallbackInfo(ipfixPrinter));
	startAggregator(ipfixAggregator);
			
	debug("starting collector");
	IpfixReceiver* ipfixReceiver = createIpfixReceiver(1500);
	addIpfixReceiverCallbacks(ipfixReceiver, getAggregatorCallbackInfo(ipfixAggregator));
	startIpfixReceiver(ipfixReceiver);

	debug("Listening on Port 1500. Hit Ctrl+C to quit");
	while (mayRun) {
		stopIpfixReceiver(ipfixReceiver);
		pollAggregator(ipfixAggregator);
		startIpfixReceiver(ipfixReceiver);
		sleep(1);
		}
	debug("Stopping threads and tidying up.");

	debug("stopping collector");
	stopIpfixReceiver(ipfixReceiver);
	destroyIpfixReceiver(ipfixReceiver);

	debug("stopping aggregator");
	stopAggregator(ipfixAggregator);
	destroyAggregator(ipfixAggregator);

	debug("stopping printer");
	stopIpfixPrinter(ipfixPrinter);
	destroyIpfixPrinter(ipfixPrinter);

	debug("stopping exporter");
	stopIpfixSender(ipfixSender);
	destroyIpfixSender(ipfixSender);

	debug("deinitializing collectors");
	deinitializeIpfixReceivers();

	debug("deinitializing aggregators");
	deinitializeAggregators();
	
	debug("deinitializing printers");
	deinitializeIpfixPrinters();

	debug("deinitializing exporters");
	deinitializeIpfixSenders();	
			
	return 0;
	}

