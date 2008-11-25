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


#include "concentrator/IpfixCollector.hpp"
#include "concentrator/IpfixParser.hpp"
#include "concentrator/IpfixPacketProcessor.hpp"
#include "concentrator/IpfixReceiverUdpIpV4.hpp"
#include "concentrator/IpfixReceiverSctpIpV4.hpp"
#include "concentrator/IpfixPrinter.hpp"
#include "reconf/ConnectionQueue.h"

#include "common/msg.h"

#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#define DEFAULT_LISTEN_PORT 1500

void usage()
{
	msg(MSG_FATAL, "Usage: testCollector [{udp|sctp} <port>]");
}

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
	std::string proto = "udp";

	msg_setlevel(MSG_DEBUG);
	
	signal(SIGINT, sigint);

	if (!(argc == 1 || argc == 2 || argc == 3)) {
		usage();
		return -1;
	}
	
	if (argc == 2)
		proto = argv[1];
	if (argc == 3)
		lport = atoi(argv[2]);

	IpfixReceiver* ipfixReceiver = 0;
	if (proto == "udp") {
		msg(MSG_INFO, "Creating UDP listener on port %i", lport);
		ipfixReceiver = new IpfixReceiverUdpIpV4(lport);
	} else if (proto == "sctp") {
#ifdef SUPPORT_SCTP
		ipfixReceiver = new IpfixReceiverSctpIpV4(lport, "127.0.0.1");
#else
		msg(MSG_FATAL, "testcollector has been compiled without sctp support");
		return -1;
#endif
	} else {
		msg(MSG_FATAL, "Protocol %s is not supported as a transport protocol for IPFIX data", proto.c_str()); 
		return -1;
	}

	IpfixCollector collector(ipfixReceiver);
	ConnectionQueue<IpfixRecord*> queue(100);
	IpfixPrinter printer;

	collector.connectTo(&queue);
	queue.connectTo(&printer);

	printer.start();
	queue.start();
	collector.start();

	msg(MSG_DIALOG, "Hit Ctrl+C to quit");
	pause();
	msg(MSG_DIALOG, "Stopping threads and tidying up.\n");

	msg(MSG_DIALOG, "stopping collector\n");
	collector.shutdown();
	queue.shutdown();
	printer.shutdown();

	return 0;
}

