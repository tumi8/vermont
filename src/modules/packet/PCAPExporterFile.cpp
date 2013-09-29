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

#include "PCAPExporterFile.h"

#include "modules/packet/Packet.h"

#include <sstream>

PCAPExporterFile::PCAPExporterFile(const std::string& file)
	: fileName(file),
	  dummy(NULL),
	  statPktsForwarded(0),
	  statBytesForwarded(0)
{
}

PCAPExporterFile::~PCAPExporterFile()
{
}

void PCAPExporterFile::performStart()
{
	char errbuf[PCAP_ERRBUF_SIZE];
	dummy = pcap_open_dead(link_type, snaplen);
	if (!dummy) {
		THROWEXCEPTION("Could not open dummy device: %s", errbuf);
	}
	dumper = pcap_dump_open(dummy, fileName.c_str());
	if (!dumper) {
		THROWEXCEPTION("Could not open dump file: %s", errbuf);
	}
}

void PCAPExporterFile::performShutdown()
{
	if (dumper) {
		if (-1 == pcap_dump_flush(dumper)) {
			msg(MSG_FATAL, "PCAPExporterFile: Could not flush dump file");
		}
		pcap_dump_close(dumper);
	}
}

void PCAPExporterFile::receive(Packet* packet)
{
	writePCAP(packet);
	
	statBytesForwarded += packet->data_length;
	statPktsForwarded++;
}

/**
 * statistics function called by StatisticsManager
 */
std::string PCAPExporterFile::getStatisticsXML(double interval)
{
	ostringstream oss;
	oss << "<forwarded type=\"packets\">" << statPktsForwarded << "</forwarded>";
	oss << "<forwarded type=\"bytes\">" << statBytesForwarded << "</forwarded>";
	return oss.str();
}
