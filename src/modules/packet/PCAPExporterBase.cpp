/*
 * Vermont PCAP Exporter
 * Copyright (C) 2009 Vermont Project
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

#include "PCAPExporterBase.h"

#include "modules/packet/Packet.h"

PCAPExporterBase::PCAPExporterBase()
	: link_type(0), snaplen(PCAP_MAX_CAPTURE_LENGTH), dumper(NULL)
{
}

PCAPExporterBase::~PCAPExporterBase()
{
}

void PCAPExporterBase::setDataLinkType(int type)
{
	link_type = type;
}

void PCAPExporterBase::setSnaplen(int len)
{
	snaplen = len;
}

void PCAPExporterBase::writePCAP(Packet* packet)
{
	static struct pcap_pkthdr packetHeader;
	packetHeader.ts = packet->timestamp;
	packetHeader.caplen = packet->data_length;
	packetHeader.len = packet->pcapPacketLength;
	pcap_dump((unsigned char*)dumper, &packetHeader, packet->layer2Start);
	packet->removeReference();
}

