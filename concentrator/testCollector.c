/** \file
 * Separate Program to test the collector
 * Dumps received flows to stdout
 */

#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "rcvIpfix.h"
#include "printIpfix.h"
#include "common.h"

void sigint() {
	}

int main(int argc, char *argv[]) {
	signal(SIGINT, sigint);

	initializeIpfixPrinters();

	initializeIpfixReceivers();

	IpfixPrinter* ipfixPrinter = createIpfixPrinter();
	startIpfixPrinter(ipfixPrinter);

	IpfixReceiver* ipfixReceiver = createIpfixReceiver(1501);
	addIpfixReceiverCallbacks(ipfixReceiver, getIpfixPrinterCallbackInfo(ipfixPrinter));
	startIpfixReceiver(ipfixReceiver);

	debug("Listening on Port 1501. Hit Ctrl+C to quit");
	pause();
	debug("Stopping threads and tidying up.");
	
	stopIpfixReceiver(ipfixReceiver);
	destroyIpfixReceiver(ipfixReceiver);
 	deinitializeIpfixReceivers();	

	stopIpfixPrinter(ipfixPrinter);
	destroyIpfixPrinter(ipfixPrinter);
	deinitializeIpfixPrinters();

	return 0;
	}

