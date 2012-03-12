/*
  VERMONT 
 * Copyright (C) 2012 Lothar Braun <braun@net.in.tum.de>
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
 */

#include "FlowLenAnalyzer.h"
#include "modules/ipfix/Connection.h"

#include <errno.h>
#include <string.h>

FlowLenAnalyzer::FlowLenAnalyzer(std::string& fFile, std::string& bFile, std::vector<uint64_t>& bs)
	:  flowFilename(fFile), binsFilename(bFile), bins(bs)
{
	// initialize flowsize output file
	flowOutstream.open(flowFilename.c_str(), ios_base::out | ios_base::trunc);
	if (not flowOutstream) {
		THROWEXCEPTION("FlowLenAnalyzer: Could not open output file \"%s\" for writing: %s", flowFilename.c_str(), strerror(errno));
	}
	flowOutstream << "# connstarttime (msec since 1970) packets bytes " << std::endl;

	// initialize binsize output file
	binsOutstream.open(binsFilename.c_str(), ios_base::out | ios_base::trunc);
	if (not binsOutstream) {
		THROWEXCEPTION("FlowLenAnalyzer: Could not open output file \"%s\" for writing: %s", binsFilename.c_str(), strerror(errno));
	}
	binsOutstream << "# binsize bytes" << std::endl;

	// initialize binmap
	for (size_t i = 0; i != bins.size(); ++i) {
		binStats[bins[i]] = 0;
	}
}

FlowLenAnalyzer::~FlowLenAnalyzer()
{
	flowOutstream.close();
	msg(MSG_INFO, "Writing bins information");
	for (std::map<uint64_t, uint64_t>::const_iterator i = binStats.begin(); i != binStats.end(); ++i) {
		binsOutstream << i->first << " " << i->second << std::endl;
	}
	binsOutstream.close();
}

void FlowLenAnalyzer::onDataRecord(IpfixDataRecord* record)
{
	uint64_t packetCount = 0;
	uint64_t byteCount = 0;
	Connection c(record);
       
	if (c.srcIP == 0) {
		// We have an IPv6 record. Do not record any stats about them
		goto out;
	}
	
	byteCount = ntohll(c.srcOctets) + ntohll(c.dstOctets);
	packetCount = ntohll(c.srcPackets) + ntohll(c.dstPackets);
	flowOutstream << c.srcTimeStart << " " << packetCount << " " << byteCount << std::endl; 

	// put data into the bins
	for (size_t i = 0; i != bins.size(); ++i) {
		// if flow is bigger than the bin, only count the bin size to the counter
		// otherwise record the flow len. for the last bin, record the complete flow size
		if (i == bins.size() - 1) {
			binStats[bins[i]] += byteCount;
		} else {
			binStats[bins[i]] += std::min(byteCount, bins[i]);
		}
	}
 out:
	// release this record
	record->removeReference();
	return ;
}


std::string FlowLenAnalyzer::getStatistics()
{
	return "";
}

std::string FlowLenAnalyzer::getStatisticsXML()
{
	return "";
}
