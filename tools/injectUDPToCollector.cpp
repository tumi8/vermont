/*
 * IPFIX/NetFlow payload replay
 * Copyright (C) 2012 Luca BRUNO <lucab@debian.org>
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
 * Simple program to ease testing the collector
 * Given a pcap netflow/ipfix dump, resend it on specified host:port
 * (where collector is listening)
 */



#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <getopt.h>
#include <string>
#include <iostream>
#include <pcap.h>

#include <netinet/in.h>
#include <netinet/ip.h>

#if defined(__FreeBSD__) || defined(__APPLE__)
#include <osdep/freebsd/iphdrs.h>
#else
#include <netinet/udp.h>
#endif

#include <net/if.h>
#include <netinet/if_ether.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define DEFAULT_PORT 2055

void usage(const char *argv0)
{
	fprintf(stderr,"Usage: %s \n",argv0);
	fprintf(stderr," --port,-p		Port number to use\n");
	fprintf(stderr," --destination,-d	Collector IPv4 address.\n");
}


int main(int argc, char *argv[]) {

	int lport = DEFAULT_PORT;
	std::string proto = "udp";
	std::string host = "127.0.0.1";

	int c;

	while (1) {
		static struct option long_options[] = {
			{"help", no_argument, 0, 'h'},
			{"destination", required_argument, 0, 'd'},
			{"port", required_argument, 0, 'p'},
			{0,0,0,0}
		};

		int option_index = 0;

		c = getopt_long(argc, argv, "d:p:", long_options, &option_index);

		if (c==-1) break;
		switch(c) {
			case '?':
			case 'h':
					usage(argv[0]);
					return -1;
					break;
			case 'p':
				char *endptr;
				lport = strtol(optarg,&endptr,10);
				if (*endptr != '\0' || lport <= 0 || lport > (1<<16) - 1) {
					std::cerr << "E: Illegal port number!" << std::endl;
					usage(argv[0]); return -1;
				}
				break;
			case 'd':
				host = std::string(optarg);
				break;
			default:
				abort();
		}
	}

	if (optind != argc-1) {
		std::cerr << "E: Missing mandatory argument: pcap filename!" << std::endl;
		usage(argv[0]); return -1;
	}


    std::string file = argv[optind];

    char errbuff[PCAP_ERRBUF_SIZE];
    pcap_t * pcap = pcap_open_offline(file.c_str(), errbuff);

    struct pcap_pkthdr *header;

    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    struct sockaddr_in saddrCollector;
    int slen=sizeof(saddrCollector);
    memset(&saddrCollector, 0, slen);
    saddrCollector.sin_family = AF_INET;
    saddrCollector.sin_port = htons(lport);
    if (inet_aton(host.c_str(), &saddrCollector.sin_addr)==0) {
	   std::cerr << "E: Bad collector address!" << std::endl;;
	   exit(1);
    }

    const u_char *packet;
    int packetCount=0;
    while (pcap_next_ex(pcap, &header, &packet) >= 0)
    {
    	struct ip *ip;
    	struct udphdr *udp;
    	unsigned int IP_header_length;
    	unsigned int capture_len = header->caplen;

    	packetCount++;
    	/* For simplicity, we assume Ethernet + IPv4 */
    	if (capture_len < sizeof(struct ether_header))
    		{
    		// Smaller than an ethernet frame, skip it
    		std::cout << "Packet #" << packetCount << " too small, skipping..." << std::endl;
    		continue;
    		}

    	packet += sizeof(struct ether_header);
    	capture_len -= sizeof(struct ether_header);

    	if (capture_len < sizeof(struct ip))
    		{
    		// Smaller than an IP header, skip it
    		std::cout << "Packet #" << packetCount << " too small, skipping..." << std::endl;
    		continue;
    		}

    	ip = (struct ip*) packet;
    	IP_header_length = ip->ip_hl * 4;

    	if (capture_len < IP_header_length)
    		{
    		// Smaller than an IP declared length, skip it
    		std::cout << "Packet #" << packetCount << " too small, skipping..." << std::endl;
    		continue;
    		}

    	if (ip->ip_p != IPPROTO_UDP)
    		{
    		// Non UDP packet, skip it
    		std::cout << "Packet #" << packetCount << " not UDP, skipping..." << std::endl;
    		continue;
    		}

    	packet += IP_header_length;
    	capture_len -= IP_header_length;

    	if (capture_len < sizeof(struct udphdr))
    		{
    		// Smaller than an UDP header, skip it
    		std::cout << "Packet #" << packetCount << " too small, skipping..." << std::endl;
    		continue;
    		}

    	udp = (struct udphdr*) packet;
    	printf("[#%d] Got an UDP packet, original src_port=%d dst_port=%d length=%d, replaying...\n",
    	    		packetCount,
    				ntohs(udp->source),
    	    		ntohs(udp->dest),
    	    		ntohs(udp->len));


    	// This is our presumed IPFIX/NetFlow payload
    	int len = ntohs(udp->len)-sizeof(struct udphdr);
    	packet += sizeof(struct udphdr);
    	sendto(fd, packet, len, 0, (const sockaddr *) &saddrCollector, sizeof(saddrCollector));

    }
	return 0;
}

