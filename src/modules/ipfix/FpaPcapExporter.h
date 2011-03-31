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
#ifndef _FPA_PCAP_EXPORTER_H_
#define _FPA_PCAP_EXPORTER_H_

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


typedef struct pcap_hdr_s {
        uint32_t magic_number;   /* magic number */
        uint16_t version_major;  /* major version number */
        uint16_t version_minor;  /* minor version number */
        int32_t  thiszone;       /* GMT to local correction */
        uint32_t sigfigs;        /* accuracy of timestamps */
        uint32_t snaplen;        /* max length of captured packets, in octets */
        uint32_t network;        /* data link type */
} pcap_hdr_t;

typedef struct pcaprec_hdr_s {
        uint32_t ts_sec;         /* timestamp seconds */
        uint32_t ts_usec;        /* timestamp microseconds */
        uint32_t incl_len;       /* number of octets of packet saved in file */
        uint32_t orig_len;       /* actual length of packet */
} pcaprec_hdr_t;


class FpaPcapExporter : public Module, public IpfixRecordDestination, public Source<NullEmitable*>
{
public:
	FpaPcapExporter(const std::string& file);
	~FpaPcapExporter();

	virtual void onDataRecord(IpfixDataRecord* record);
	virtual void performStart();
	virtual void performShutdown();

	void setSnaplen(int len);

private:
	static void* pcapExporterSink(void* data);

	std::string fileName;
	FILE* pcapfile;
	int snaplen;

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
};

#endif
