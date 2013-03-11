/*
 * FPA PCAP Exporter
 * Copyright (C) 2009 Tobias Limmer <http://www.cs.fau.de/~limmer>
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


#include "FpaPacketGenerator.h"
#include "Connection.h"

InstanceManager<Packet> FpaPacketGenerator::packetManager("Packet");


FpaPacketGenerator::FpaPacketGenerator()
{
	// prepare headers for TCP packets
	bzero(&tcpHeader, sizeof(FullTcpHeader));
	tcpHeader.ether.type = 0x0008;
	udpHeader.ether.type = 0x0008;
	tcpHeader.ip.ihl = sizeof(tcpHeader.ip)/4;
	udpHeader.ip.ihl = sizeof(udpHeader.ip)/4;
	tcpHeader.ip.version = 4;
	udpHeader.ip.version = 4;
	//tcpHeader.ip.tot_len
	//tcpHeader.ip.id
	//tcpHeader.ip.frag_off
	tcpHeader.ip.ttl = 1;
	udpHeader.ip.ttl = 1;
	tcpHeader.ip.protocol = 6;
	udpHeader.ip.protocol = 17;
	//tcpHeader.ip.check
	//tcpHeader.ip.saddr
	//tcpHeader.ip.daddr
	//tcpHeader.tcp.source
	//tcpHeader.tcp.dest
	tcpHeader.tcp.seq = 1;
	tcpHeader.tcp.ack_seq = 1;
	//tcpHeader.tcp.th_x2
	tcpHeader.tcp.doff = sizeof(tcpHeader.tcp)/4;
	//tcpHeader.tcp.th_flags
	//tcpHeader.tcp.th_win
	//tcpHeader.tcp.th_sum
	//tcpHeader.tcp.th_urp
	//udpHeader.udp.source
	//udpHeader.udp.dest
	//udpHeader.udp.len
	//udpHeader.udp.check
	tcpDataSegments[0] = reinterpret_cast<char*>(&tcpHeader);
	tcpDataSegments[2] = NULL;
	tcpSegmentLengths[0] = sizeof(tcpHeader);
	tcpSegmentLengths[2] = 0;
	udpDataSegments[0] = reinterpret_cast<char*>(&udpHeader);
	udpDataSegments[2] = NULL;
	udpSegmentLengths[0] = sizeof(udpHeader);
	udpSegmentLengths[2] = 0;
}

FpaPacketGenerator::~FpaPacketGenerator()
{
}

void FpaPacketGenerator::performStart()
{
}

void FpaPacketGenerator::performShutdown()
{
}

/**
 * Takes an IPFIX record, and tries to create a single packet for the record (two packets if it is bidirectional)
 * Payload of the packet is contained in the FPA payload field of the flow
 * This is done by using two prepared buffers that contain a UDP and TCP header stack (tcpHeader and udpHeader)
 * ATTENTION: checksums are not calculated at all, as well as other TCP header fields that are connection specific
 */
void FpaPacketGenerator::onDataRecord(IpfixDataRecord* record)
{
	// only treat non-Options Data Records (although we cannot be sure that there is a Flow inside)
	if((record->templateInfo->setId != TemplateInfo::NetflowTemplate)
		&& (record->templateInfo->setId != TemplateInfo::IpfixTemplate)
		&& (record->templateInfo->setId != TemplateInfo::IpfixDataTemplate)) {
		record->removeReference();
		return;
	}

	Connection c(record);

	struct timeval tv;

	if (c.protocol==6) {
		uint16_t totlen = sizeof(tcpHeader)+c.srcPayloadLen;
		if (c.srcPayloadLen>0) {
			// copy normal direction
			tcpHeader.ip.tot_len = htons(totlen-sizeof(tcpHeader.ether));
			tcpHeader.ip.saddr = c.srcIP;
			tcpHeader.ip.daddr = c.dstIP;
			tcpHeader.tcp.source = c.srcPort;
			tcpHeader.tcp.dest = c.dstPort;

			tcpSegmentLengths[1] = totlen-sizeof(tcpHeader);
			tcpDataSegments[1] = c.srcPayload;
			tv.tv_sec = c.srcTimeEnd/1000;
			tv.tv_usec = (c.srcTimeEnd%1000)*1000;
			Packet* p = packetManager.getNewInstance();
			// tcpheader contains a standard 14 bytes ethernet header (see FpaPacketGenerator.h)
			p->init(tcpDataSegments, tcpSegmentLengths,	tv, 0, totlen, 14);
			send(p);
		}
		totlen = sizeof(tcpHeader)+c.dstPayloadLen;
		if (c.dstPayloadLen>0) {
			// copy reverse direction
			tcpHeader.ip.tot_len = htons(totlen-sizeof(tcpHeader.ether));
			tcpHeader.ip.saddr = c.dstIP;
			tcpHeader.ip.daddr = c.srcIP;
			tcpHeader.tcp.source = c.dstPort;
			tcpHeader.tcp.dest = c.srcPort;

			tcpSegmentLengths[1] = totlen-sizeof(tcpHeader);
			tcpDataSegments[1] = c.dstPayload;
			tv.tv_sec = c.dstTimeEnd/1000;
			tv.tv_usec = (c.dstTimeEnd%1000)*1000;
			Packet* p = packetManager.getNewInstance();
			// tcpheader contains a standard 14 bytes ethernet header (see FpaPacketGenerator.h)
			p->init(tcpDataSegments, tcpSegmentLengths,	tv, 0, totlen, 14);
			send(p);
		}
	} else if (c.protocol==17) {
		uint16_t totlen = sizeof(udpHeader)+c.srcPayloadLen;
		if (c.srcPayloadLen>0) {
			// copy normal direction
			udpHeader.ip.tot_len = htons(totlen-sizeof(udpHeader.ether));
			udpHeader.ip.saddr = c.srcIP;
			udpHeader.ip.daddr = c.dstIP;
			udpHeader.udp.source = c.srcPort;
			udpHeader.udp.dest = c.dstPort;
			udpHeader.udp.len = htons(totlen-sizeof(udpHeader)+sizeof(udpHeader.udp));

			udpSegmentLengths[1] = totlen-sizeof(udpHeader);
			udpDataSegments[1] = c.srcPayload;
			tv.tv_sec = c.srcTimeEnd/1000;
			tv.tv_usec = (c.srcTimeEnd%1000)*1000;
			Packet* p = packetManager.getNewInstance();
			// udpheader contains a standard 14 bytes ethernet header (see FpaPacketGenerator.h)
			p->init(udpDataSegments, udpSegmentLengths,	tv, 0, totlen, 14);
			send(p);
		}
		totlen = sizeof(udpHeader)+c.dstPayloadLen;
		if (c.dstPayloadLen>0) {
			// copy reverse direction
			udpHeader.ip.tot_len = htons(totlen-sizeof(udpHeader.ether));
			udpHeader.ip.saddr = c.dstIP;
			udpHeader.ip.daddr = c.srcIP;
			udpHeader.udp.source = c.dstPort;
			udpHeader.udp.dest = c.srcPort;
			udpHeader.udp.len = htons(totlen-sizeof(udpHeader)+sizeof(udpHeader.udp));

			udpSegmentLengths[1] = totlen-sizeof(udpHeader);
			udpDataSegments[1] = c.dstPayload;
			tv.tv_sec = c.dstTimeEnd/1000;
			tv.tv_usec = (c.dstTimeEnd%1000)*1000;
			Packet* p = packetManager.getNewInstance();
			// udpheader contains a standard 14 bytes ethernet header (see FpaPacketGenerator.h)
			p->init(udpDataSegments, udpSegmentLengths,	tv, 0, totlen, 14);
			send(p);

		}
	}
	record->removeReference();
}

