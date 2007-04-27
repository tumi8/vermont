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
 * Separate Program to test collector, printer and sender
 * Dumps received flows to stdout
 */

#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "IpfixReceiver.hpp"
#include "IpfixPrinter.hpp"
#include "IpfixSender.hpp"
#include "common.hpp"

#define DEFAULT_LISTEN_PORT 1500
#define DEFAULT_TALK_IP "127.0.0.1"
#define DEFAULT_TALK_PORT 1501

#define DEFAULT_SOURCE_ID 4711

void sigint(int) {
}

int main(int argc, char* argv[]) {
	int lport = DEFAULT_LISTEN_PORT;
	char* tip = DEFAULT_TALK_IP;
	int tport = DEFAULT_TALK_PORT;

	signal(SIGINT, sigint);

	if (argc > 1) lport=atoi(argv[1]);
	if (argc > 2) tip=argv[2];
	if (argc > 3) tport=atoi(argv[3]);

	IpfixReceiver ipfixReceiver(lport);
	IpfixPrinter ipfixPrinter;
	IpfixSender ipfixSender(DEFAULT_SOURCE_ID, tip, tport);

	ipfixReceiver.addFlowSink(&ipfixSender);
	ipfixReceiver.addFlowSink(&ipfixPrinter);

	ipfixSender.start();
	ipfixPrinter.start();
	ipfixReceiver.start();

	debugf("0.0.0.0:%d => %s:%d", lport, tip, tport);
	debug("Forwarding all Templates and Data Records. Press Ctrl+C to quit.");
	pause();
	debug("Stopping threads and tidying up.");

	ipfixReceiver.stop();
	ipfixPrinter.stop();
	ipfixSender.stop();

	return 0;
}

