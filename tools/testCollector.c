/** \file
 * Separate Program to test the collector
 * Dumps received flows to stdout
 */

#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "rcvIpfix.h"
#include "printIpfix.h"
#include "../msg.h"

#define DEFAULT_LISTEN_PORT 1500

void sigint() {
	}

int main(int argc, char *argv[]) {

        int lport = DEFAULT_LISTEN_PORT;

        msg_setlevel(MSG_DEFAULT);

        signal(SIGINT, sigint);

        if(argv[1]) {
                lport=atoi(argv[1]);
        }

	initializeIpfixPrinters();

	initializeIpfixCollectors();

	IpfixPrinter* ipfixPrinter = createIpfixPrinter();
	startIpfixPrinter(ipfixPrinter);

	IpfixCollector* ipfixCollector = createIpfixCollector();
	IpfixReceiver* ipfixReceiver = createIpfixReceiver(UDP_IPV4, lport);
	addIpfixReceiver(ipfixCollector, ipfixReceiver);

	IpfixPacketProcessor* ipfixPacketProcessor = createIpfixPacketProcessor();
	IpfixParser* ipfixParser = createIpfixParser();

	/* (not in this branch of rcvIpfix)
	if (argc > 2) {
		msg(MSG_DIALOG, "Adding %s to list of authorized hosts", argv[2]);
		addIpfixReceiverAuthorizedHost(ipfixReceiver, argv[2]);
		}
	*/

	addIpfixParserCallbacks(ipfixParser, getIpfixPrinterCallbackInfo(ipfixPrinter));
	setIpfixParser(ipfixPacketProcessor, ipfixParser);
	addIpfixPacketProcessor(ipfixCollector, ipfixPacketProcessor);

	startIpfixCollector(ipfixCollector);

	msg(MSG_DIALOG, "Listening on %d. Hit Ctrl+C to quit", lport);
	pause();
	msg(MSG_DIALOG, "Stopping threads and tidying up.");
	
	stopIpfixCollector(ipfixCollector);
	destroyIpfixCollector(ipfixCollector);
 	deinitializeIpfixReceivers();	

	stopIpfixPrinter(ipfixPrinter);
	destroyIpfixPrinter(ipfixPrinter);
	deinitializeIpfixPrinters();

	return 0;
	}

