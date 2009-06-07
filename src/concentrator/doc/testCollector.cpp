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
#include "IpfixReceiver.hpp"
#include "IpfixPrinter.hpp"
#include "common.hpp"

#define DEFAULT_LISTEN_PORT 1500

void sigint(int) {
}

int main(int argc, char *argv[]) {

	int lport = DEFAULT_LISTEN_PORT;

	signal(SIGINT, sigint);

	if(argv[1]) {
		lport=atoi(argv[1]);
	}

	IpfixPrinter ipfixPrinter;
	ipfixPrinter.start();

	IpfixReceiver ipfixReceiver(lport);

	if (argc > 2) {
		debugf("Adding %s to list of authorized hosts", argv[2]);
		ipfixReceiver.addAuthorizedHost(argv[2]);
	}

	ipfixReceiver.addFlowSink(&ipfixPrinter);
	ipfixReceiver.start();

	debugf("Listening on %d. Hit Ctrl+C to quit", lport);
	pause();
	debug("Stopping threads and tidying up.");

	debug("stopping collector");
	ipfixReceiver.stop();

	debug("stopping printer");
	ipfixPrinter.stop();

	return 0;
}

