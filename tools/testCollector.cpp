/*
 * IPFIX Concentrator Module Library
 * Copyright (C) 2004 Christoph Sommer <http://www.deltadevelopment.de/users/christoph/ipfix/>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

/** \file
 * Separate Program to test the collector
 * Dumps received flows to stdout
 */

#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "IpfixCollector.hpp"
#include "IpfixParser.hpp"
#include "IpfixPacketProcessor.hpp"
#include "IpfixReceiverUdpIpV4.hpp"
#include "IpfixPrinter.hpp"
#include "../msg.h"

#define DEFAULT_LISTEN_PORT 1500

void sigint(int) {
    static bool shutdownInitiated = false;

    if (shutdownInitiated) {
        msg(MSG_DIALOG, "second signal received, shutting down the hard way!");
        exit(2);
    }

    shutdownInitiated = true;
}

int main(int argc, char *argv[]) {

	int lport = DEFAULT_LISTEN_PORT;

	msg_setlevel(MSG_DEFAULT);

	signal(SIGINT, sigint);

	if(argv[1]) {
		lport=atoi(argv[1]);
	}

	IpfixPrinter ipfixPrinter;
	ipfixPrinter.start();
	ipfixPrinter.runSink();

	IpfixReceiverUdpIpV4 ipfixReceiver(lport);
	/* (not in this branch of rcvIpfix)
	if (argc > 2) {
		msg(MSG_DIALOG, "Adding %s to list of authorized hosts", argv[2]);
		ipfixReceiver.addAuthorizedHost(argv[2]);
	}
	*/

	IpfixParser ipfixParser;
	ipfixParser.addFlowSink(&ipfixPrinter);

	IpfixCollector ipfixCollector;
	ipfixCollector.addIpfixReceiver(&ipfixReceiver);
	ipfixCollector.addIpfixPacketProcessor(&ipfixParser);
	ipfixCollector.start();

	msg(MSG_DIALOG, "Listening on %d. Hit Ctrl+C to quit\n", lport);
	pause();
	msg(MSG_DIALOG, "Stopping threads and tidying up.\n");

	msg(MSG_DIALOG, "stopping collector\n");
	ipfixReceiver.stop();

	msg(MSG_DIALOG, "stopping printer\n");
	ipfixPrinter.stop();

	return 0;
}

