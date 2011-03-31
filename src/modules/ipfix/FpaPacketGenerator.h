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
#ifndef _FPA_PACKET_GENERATOR_H_
#define _FPA_PACKET_GENERATOR_H_

#include "core/Module.h"
#include "IpfixRecordDestination.h"

#include <common/msg.h>

#include <string>
#include <pcap.h>

#if defined(__FreeBSD__) || defined(__APPLE__)
// FreeBSD does not have a struct iphdr. Furthermore, the struct members in
// struct udphdr and tcphdr differ from the linux version. We therefore need
// to define them
#include <osdep/freebsd/iphdrs.h>
#else
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#endif


class FpaPacketGenerator : public Module, public IpfixRecordDestination, public Source<Packet*>
{
public:
	FpaPacketGenerator();
	~FpaPacketGenerator();

	virtual void onDataRecord(IpfixDataRecord* record);
	virtual void performStart();
	virtual void performShutdown();

private:
	static void* pcapExporterSink(void* data);

	struct etherhdr {
		char dstaddr[6];
		char srcaddr[6];
		uint16_t type;
	};

	struct FullTcpHeader {
		etherhdr ether;
		iphdr ip;
		tcphdr tcp;
	} __attribute__ (( packed )) ;

	struct FullUdpHeader {
		etherhdr ether;
		iphdr ip;
		udphdr udp;
	} __attribute__ (( packed )) ;

	FullTcpHeader tcpHeader;
	FullUdpHeader udpHeader;

	char* tcpDataSegments[3];
	uint32_t tcpSegmentLengths[3];
	char* udpDataSegments[3];
	uint32_t udpSegmentLengths[3];

	static InstanceManager<Packet> packetManager;
};

#endif
