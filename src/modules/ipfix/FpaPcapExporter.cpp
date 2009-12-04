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


#include "FpaPcapExporter.h"
#include "Connection.h"


FpaPcapExporter::FpaPcapExporter(const std::string& file)
	: fileName(file), pcapfile(NULL), snaplen(PCAP_MAX_CAPTURE_LENGTH)
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
}

FpaPcapExporter::~FpaPcapExporter()
{
}

void FpaPcapExporter::setSnaplen(int len)
{
	snaplen = len;
}

void FpaPcapExporter::performStart()
{
	pcapfile = fopen(fileName.c_str(), "w");
	if (!pcapfile) {
		THROWEXCEPTION("FpaPcapExporter: Could not open dump file %s, error: %s", fileName.c_str(), strerror(errno));
	}

	pcap_hdr_t hdr;
	bzero(&hdr, sizeof(pcap_hdr_t));
	hdr.magic_number = 0xa1b2c3d4;
	hdr.version_major = 2;
	hdr.version_minor = 4;
	hdr.snaplen = snaplen;
	hdr.network = DLT_EN10MB;
	if (fwrite(&hdr, sizeof(hdr), 1, pcapfile)!=1) {
		THROWEXCEPTION("FpaPcapExporter: Failed to write global header to file %s, error: %s", fileName.c_str(), strerror(errno));
	}
}

void FpaPcapExporter::performShutdown()
{
	if (pcapfile) {
		fclose(pcapfile);
	}
}

/**
 * Takes an IPFIX record, and tries to create a single packet for the record (two packets if it is bidirectional)
 * Payload of the packet is contained in the FPA payload field of the flow
 * This is done by using two prepared buffers that contain a UDP and TCP header stack (tcpHeader and udpHeader)
 * ATTENTION: checksums are not calculated at all, as well as other TCP header fields that are connection specific
 */
void FpaPcapExporter::onDataRecord(IpfixDataRecord* record)
{
	// only treat non-Options Data Records (although we cannot be sure that there is a Flow inside)
	if((record->templateInfo->setId != TemplateInfo::NetflowTemplate) 
		&& (record->templateInfo->setId != TemplateInfo::IpfixTemplate) 
		&& (record->templateInfo->setId != TemplateInfo::IpfixDataTemplate)) {
		record->removeReference();
		return;
	}
	
	Connection c(record);

	pcaprec_hdr_t pcaphdr;
	bzero(&pcaphdr, sizeof(pcaphdr));
	//pcaphdr.ts_sec = 0;	// FIXME: here we should use timestamp contained in IPFIX record
	//pcaphdr.ts_usec = 0 // see above!

	if (c.protocol==6) {
		uint16_t totlen = sizeof(tcpHeader)+c.srcPayloadLen;
		if (totlen>snaplen) totlen = snaplen;
		if (c.srcPayloadLen>0) {
			// copy normal direction
			tcpHeader.ip.tot_len = htons(totlen-sizeof(tcpHeader.ether));
			pcaphdr.incl_len = totlen;
			pcaphdr.orig_len = totlen;
			tcpHeader.ip.saddr = c.srcIP;
			tcpHeader.ip.daddr = c.dstIP;
			tcpHeader.tcp.source = c.srcPort;
			tcpHeader.tcp.dest = c.dstPort;
			if (fwrite(&pcaphdr, sizeof(pcaphdr), 1, pcapfile)!=1) {
				THROWEXCEPTION("FpaPcapExporter: Failed to write packet header to file %s, error: %s", fileName.c_str(), strerror(errno));
			}
			if (fwrite(&tcpHeader, sizeof(tcpHeader), 1, pcapfile)!=1) {
				THROWEXCEPTION("FpaPcapExporter: Failed to write packet tcp header to file %s, error: %s", fileName.c_str(), strerror(errno));
			}
			if (fwrite(c.srcPayload, totlen-sizeof(tcpHeader), 1, pcapfile)!=1) {
				THROWEXCEPTION("FpaPcapExporter: Failed to write tcp packet payload to file %s, error: %s", fileName.c_str(), strerror(errno));
			}

		}
		totlen = sizeof(tcpHeader)+c.dstPayloadLen;
		if (totlen>snaplen) totlen = snaplen;
		if (c.dstPayloadLen>0) {
			// copy reverse direction
			tcpHeader.ip.tot_len = htons(totlen-sizeof(tcpHeader.ether));
			pcaphdr.incl_len = totlen;
			pcaphdr.orig_len = totlen;
			tcpHeader.ip.saddr = c.dstIP;
			tcpHeader.ip.daddr = c.srcIP;
			tcpHeader.tcp.source = c.dstPort;
			tcpHeader.tcp.dest = c.srcPort;
			if (fwrite(&pcaphdr, sizeof(pcaphdr), 1, pcapfile)!=1) {
				THROWEXCEPTION("FpaPcapExporter: Failed to write packet header to file %s, error: %s", fileName.c_str(), strerror(errno));
			}
			if (fwrite(&tcpHeader, sizeof(tcpHeader), 1, pcapfile)!=1) {
				THROWEXCEPTION("FpaPcapExporter: Failed to write packet tcp header to file %s, error: %s", fileName.c_str(), strerror(errno));
			}
			if (fwrite(c.dstPayload, totlen-sizeof(tcpHeader), 1, pcapfile)!=1) {
				THROWEXCEPTION("FpaPcapExporter: Failed to write tcp packet payload to file %s, error: %s", fileName.c_str(), strerror(errno));
			}
		}
	} else if (c.protocol==17) {
		uint16_t totlen = sizeof(udpHeader)+c.srcPayloadLen;
		if (totlen>snaplen) totlen = snaplen;
		if (c.srcPayloadLen>0) {
			// copy normal direction
			udpHeader.ip.tot_len = htons(totlen-sizeof(udpHeader.ether));
			pcaphdr.incl_len = totlen;
			pcaphdr.orig_len = totlen;
			udpHeader.ip.saddr = c.srcIP;
			udpHeader.ip.daddr = c.dstIP;
			udpHeader.udp.source = c.srcPort;
			udpHeader.udp.dest = c.dstPort;
			udpHeader.udp.len = htons(totlen-sizeof(udpHeader)+sizeof(udpHeader.udp));
			if (fwrite(&pcaphdr, sizeof(pcaphdr), 1, pcapfile)!=1) {
				THROWEXCEPTION("FpaPcapExporter: Failed to write packet header to file %s, error: %s", fileName.c_str(), strerror(errno));
			}
			if (fwrite(&udpHeader, sizeof(udpHeader), 1, pcapfile)!=1) {
				THROWEXCEPTION("FpaPcapExporter: Failed to write packet udp header to file %s, error: %s", fileName.c_str(), strerror(errno));
			}
			if (fwrite(c.srcPayload, totlen-sizeof(udpHeader), 1, pcapfile)!=1) {
				THROWEXCEPTION("FpaPcapExporter: Failed to write udp packet payload to file %s, error: %s", fileName.c_str(), strerror(errno));
			}
		}
		totlen = sizeof(udpHeader)+c.dstPayloadLen;
		if (totlen>snaplen) totlen = snaplen;
		if (c.dstPayloadLen>0) {
			// copy reverse direction
			udpHeader.ip.tot_len = htons(totlen-sizeof(udpHeader.ether));
			pcaphdr.incl_len = totlen;
			pcaphdr.orig_len = totlen;
			udpHeader.ip.saddr = c.dstIP;
			udpHeader.ip.daddr = c.srcIP;
			udpHeader.udp.source = c.dstPort;
			udpHeader.udp.dest = c.srcPort;
			udpHeader.udp.len = htons(totlen-sizeof(udpHeader)+sizeof(udpHeader.udp));
			if (fwrite(&pcaphdr, sizeof(pcaphdr), 1, pcapfile)!=1) {
				THROWEXCEPTION("FpaPcapExporter: Failed to write packet header to file %s, error: %s", fileName.c_str(), strerror(errno));
			}
			if (fwrite(&udpHeader, sizeof(udpHeader), 1, pcapfile)!=1) {
				THROWEXCEPTION("FpaPcapExporter: Failed to write packet udp header to file %s, error: %s", fileName.c_str(), strerror(errno));
			}
			if (fwrite(c.dstPayload, totlen-sizeof(udpHeader), 1, pcapfile)!=1) {
				THROWEXCEPTION("FpaPcapExporter: Failed to write udp packet payload to file %s, error: %s", fileName.c_str(), strerror(errno));
			}
		}
	}
	record->removeReference();
}

