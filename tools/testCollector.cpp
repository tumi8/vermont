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


#include "modules/ipfix/IpfixCollector.hpp"
#include "modules/ipfix/IpfixParser.hpp"
#include "modules/ipfix/IpfixPacketProcessor.hpp"
#include "modules/ipfix/IpfixReceiverUdpIpV4.hpp"
#include "modules/ipfix/IpfixReceiverDtlsUdpIpV4.hpp"
#include "modules/ipfix/IpfixReceiverSctpIpV4.hpp"
#include "modules/ipfix/IpfixPrinter.hpp"
#include "core/ConnectionQueue.h"
#include "common/msg.h"

#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <getopt.h>

#define DEFAULT_LISTEN_PORT 1500

void usage(const char *argv0)
{
	fprintf(stderr,"Usage: %s\n",argv0);
	fprintf(stderr," --port,-p       Port number to use\n");
	fprintf(stderr," --protocol      Either udp, sctp or dtls_over_udp\n");
	fprintf(stderr," --mode,-m	 Printing mode to use: \"line\" or \"tree\". Default: line\n");
	fprintf(stderr," --cert          The certificate to use\n");
	fprintf(stderr," --key           The private key to use\n");
	fprintf(stderr," --CAfile        A file containing trusted "
			"certificates to use during client authentication\n");
	fprintf(stderr," --CApath        The directory to use for client "
			"certificate verification. These are also used "
			"when building the server certificate chain.\n");
	fprintf(stderr," --peername      Expected FQDN of the peer.\n");
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
	std::string certificateChainFile, privateKeyFile, caFile, caPath, peername;
	IpfixPrinter::OutputType outputType = IpfixPrinter::LINE;

	msg_init();
	msg_setlevel(MSG_DEBUG);
	
	signal(SIGINT, sigint);

	int c;

	while (1) {
		static struct option long_options[] = {
			{"help", no_argument, 0, 'h'},
			{"port", required_argument, 0, 'p'},
			{"protocol", required_argument, 0, 'o'},
			{"mode", required_argument, 0, 'm'},
			{"cert", required_argument, 0, 'c'},
			{"key", required_argument, 0, 'k'},
			{"CAfile", required_argument, 0, 'a'},
			{"CApath", required_argument, 0, 'b'},
			{"peername", required_argument, 0, 'n'},
			{0,0,0,0}
		};

		int option_index = 0;

		c = getopt_long(argc, argv, "hp:m:", long_options, &option_index);

		if (c==-1) break;
		switch(c) {
			case 'h':
				usage(argv[0]); return -1;
			case 'p':
				char *endptr;
				lport = strtol(optarg,&endptr,10);
				if (*endptr != '\0' || lport <= 0 || lport > (1<<16) - 1) {
					fprintf(stderr, "illegal port number\n");
					usage(argv[0]); return -1;
				}
				break;
			case 'o':
				proto = optarg;
				break;
			case 'm':
				if (strcmp(optarg, "line") == 0) {
					outputType = IpfixPrinter::LINE;
				} else if (strcmp(optarg, "tree") == 0) {
					outputType = IpfixPrinter::TREE;
				} else {
					fprintf(stderr, "ERROR: unknown parameter to option --mode\n");
					abort();
				}
				break;
			case 'c':
				certificateChainFile = optarg;
				break;
			case 'k':
				privateKeyFile = optarg;
				break;
			case 'a':
				caFile = optarg;
				break;
			case 'b':
				caPath = optarg;
				break;
			case 'n':
				peername = optarg;
				break;
			case '?':
				break;
			default:
				abort();
		}
	}

	if (optind != argc) {
		fprintf(stderr,"unrecognized option '%s'\n",argv[optind]);
		usage(argv[0]); return -1;
	}

	IpfixReceiver* ipfixReceiver = 0;
	if (proto == "udp") {
		msg(MSG_INFO, "Creating UDP listener on port %i", lport);
		ipfixReceiver = new IpfixReceiverUdpIpV4(lport);
	} else if (proto == "dtls_over_udp") {
#ifdef SUPPORT_DTLS
		msg(MSG_INFO, "Creating DTLS over UDP listener on port %i", lport);
		ipfixReceiver = new IpfixReceiverDtlsUdpIpV4(lport,"",certificateChainFile,privateKeyFile,caFile,caPath);
#else
		msg(MSG_FATAL, "testcollector has been compiled without dtls/openssl support");
		return -1;
#endif
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
	IpfixPrinter printer(outputType);

	collector.connectTo(&queue);
	queue.connectTo(&printer);

	printer.start();
	queue.start();
	collector.start();

	msg(MSG_DIALOG, "Hit Ctrl+C to quit");
	pause();
	msg(MSG_DIALOG, "Stopping threads and tidying up.\n");

	msg(MSG_DIALOG, "Stopping collector\n");
	collector.shutdown();
	queue.shutdown();
	printer.shutdown();

	return 0;
}

