/*
 * IPFIX Netflow Exporter
 * Copyright (C) 2011 Mario Volke
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

#include "IpfixSimpleTcpExporter.h"
#include "Connection.h"

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

IpfixSimpleTcpExporter::IpfixSimpleTcpExporter(string hostname, uint16_t port)
	: destPort(port)
{
	siDest.sin_family = AF_INET;
	siDest.sin_port = htons(destPort);
	struct hostent* host = gethostbyname(hostname.c_str());
	if(host==NULL) {
		THROWEXCEPTION("IpfixSimpleTcpExporter: failed to resolve host '%s' (%s)", hostname.c_str(), strerror(errno));
	}
	siDest.sin_addr = *reinterpret_cast<struct in_addr*>(host->h_addr_list[0]);
}

IpfixSimpleTcpExporter::~IpfixSimpleTcpExporter()
{
}

void IpfixSimpleTcpExporter::performStart()
{
	if((sockfd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))==-1) {
		THROWEXCEPTION("IpfixSimpleTcpExporter: failed to set up TCP socket (%s)", strerror(errno));
	}

	if(connect(sockfd, (struct sockaddr *)&siDest, sizeof(struct sockaddr)) < 0){
		THROWEXCEPTION("IpfixSimpleTcpExporter: could not connect", strerror(errno));
	}
}

void IpfixSimpleTcpExporter::performShutdown()
{
	close(sockfd);
}

/**
 * Put new Data Record in outbound exporter queue
 * @param rec Data Data Record
 */
void IpfixSimpleTcpExporter::onDataRecord(IpfixDataRecord* record)
{
	Connection c(record);

	// construct simple string packet
	char buf[1000];
	int len = sprintf(buf, "%s\n%s\n%hu\n%hu\n%hhu\n%llu\n%llu\n%llu\n%llu\n%llu\n%llu\n%llu\n%llu\n%u\n%u\n%hhu\n%hhu\n%u\n%llu\n%llu\n\n",
			IPToString(c.srcIP).c_str(), IPToString(c.dstIP).c_str(), ntohs(c.srcPort), ntohs(c.dstPort), c.protocol,
			(long long unsigned)ntohll(c.srcPackets), (long long unsigned)ntohll(c.dstPackets), (long long unsigned)ntohll(c.srcOctets), (long long unsigned)ntohll(c.dstOctets),
			(long long unsigned)c.srcTimeStart, (long long unsigned)c.srcTimeEnd, (long long unsigned)c.dstTimeStart, (long long unsigned)c.dstTimeEnd,
			c.srcPayloadLen, c.dstPayloadLen, c.dpaForcedExport, c.dpaReverseStart, c.dpaFlowCount, (long long unsigned)c.srcTransOctets, (long long unsigned)c.dstTransOctets);

	int sent = 0,
		tmpres;
	while(sent < len) {
		// send is also declared in Source
    	tmpres = ::send(sockfd, buf+sent, len-sent, 0);
    	if(tmpres == -1){
			msg(MSG_ERROR, "IpfixSimpleTcpExporter: WARNING, failed to send TCP packet (%s)", strerror(errno));
			return;
    	}
    	sent += tmpres;
  	}
}
