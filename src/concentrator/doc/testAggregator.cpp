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
 * Separate Program to test the Aggregator module
 * Dumps received flows to stdout
 */

#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include "common.hpp"
#include "IpfixReceiver.hpp"
#include "IpfixAggregator.hpp"
#include "IpfixSender.hpp"
#include "IpfixPrinter.hpp"

int mayRun;

void sigint(int) {
	mayRun = 0;
}

int main(int argc, char *argv[]) {
	mayRun = 1;
	signal(SIGINT, sigint);

	debug("starting exporter");
	IpfixSender ipfixSender(0xDEADBEEF, "127.0.0.1", 1501);
	ipfixSender.start();

	debug("starting printer");
	IpfixPrinter ipfixPrinter;
	ipfixPrinter.start();

	debug("starting aggregator");
	IpfixAggregator ipfixAggregator("aggregation_rules.conf", 5, 15);
	ipfixAggregator.addFlowSink(&ipfixSender);
	ipfixAggregator.addFlowSink(&ipfixPrinter);
	ipfixAggregator.start();

	debug("starting collector");
	IpfixReceiver ipfixReceiver(1500);
	ipfixReceiver.addFlowSink(&ipfixAggregator);
	ipfixReceiver.start();

	debug("Listening on Port 1500. Hit Ctrl+C to quit");
	while (mayRun) {
		ipfixAggregator.poll();
		sleep(1);
	}
	debug("Stopping threads and tidying up.");

	debug("stopping collector");
	ipfixReceiver.stop();

	debug("stopping aggregator");
	ipfixAggregator.stop();

	debug("stopping printer");
	ipfixPrinter.stop();

	debug("stopping exporter");
	ipfixSender.stop();

	return 0;
}

