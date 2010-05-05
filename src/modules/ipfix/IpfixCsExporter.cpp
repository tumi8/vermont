/*
 * IPFIX Carmentis Exporter Module
 * Copyright (C) 2010 Vermont Project
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


#include "IpfixCsExporter.hpp"

#include "common/ipfixlolib/ipfixlolib.h"
#include "common/ipfixlolib/ipfix.h"
#include "common/msg.h"

#include <stdexcept>
#include <string.h>


/* go back to SENDER_TEMPLATE_ID_LOW if _HI is reached */
#define SENDER_TEMPLATE_ID_HI 60000

/**
 * Creates a new IPFIXCsExporter. Do not forget to call @c startIpfixCsExporter() to begin sending
 */
IpfixCsExporter::IpfixCsExporter(uint16_t observationDomainId, std::string filenamePrefix, 
		std::string destinationPath, uint32_t maximumFilesize, uint32_t maxChunkBufferTime,
		uint32_t maxChunkBufferRecords, uint32_t maxFileCreationInterval)
			: IpfixSender(observationDomainId)
{
	/*
	if (filenamePrefix != "") {
		if(addCollector(observationDomainId, filenamePrefix, destinationPath, maximumFilesize, maxChunkBufferTime, maxChunkBufferRecords, maxFileCreationInterval) != 0) {
			THROWEXCEPTION("IpfixCsExporter's Collector addition failed");
			return;
		}
	}
	*/
	msg(MSG_DEBUG, "IpfixCsExporter: running");
}

IpfixCsExporter::~IpfixCsExporter() {
}

/**
 * Add another IPFIX collector to export the stream to
 * the lowlevel stuff in handled by underlying ipfixlolib
 */
int IpfixCsExporter::addCollector(uint16_t observationDomainId, std::string filenamePrefix, 
			std::string destinationPath, uint32_t maximumFilesize, 
			uint32_t maxChunkBufferTime, uint32_t maxChunkBufferRecords,
			uint32_t maxFileCreationInterval) 
{
	/*
	ipfix_exporter *ex = (ipfix_exporter *)ipfixExporter;
	*/
	if(destinationPath.at(destinationPath.length()-1) != '/') 
		destinationPath += "/";
	std::string my_filename = destinationPath + filenamePrefix; 
	if (maxFileSize < 0) maxFileSize = DEFAULTFILESIZE;
	if(maxFileSize < 64)
		 msg(MSG_ERROR, 
		   "maximum filsize < maximum message length - this could lead to serious problems");
	/*
	if(ipfix_add_collector(ex, my_filename.c_str(), maximumFilesize, DATAFILE) != 0) {
		msg(MSG_FATAL, "IpfixFileWriter: ipfix_add_collector of %s failed", my_filename.c_str());
		return -1;
	}*/

	msg(MSG_INFO, "IpfixCsExporter: adding %s to exporter", my_filename.c_str());
	msg(MSG_INFO, "IpfixCsExporter initialized with the following parameters");
	msg(MSG_INFO, "  - Basename = %s", my_filename.c_str());
	msg(MSG_INFO, "  - maximumFilesize = %d KiB" , maxFileSize);
        msg(MSG_INFO, "  - maxChunkBufferTime = %d seconds" , maxChunkBufferTime);
        msg(MSG_INFO, "  - maxChunkBufferRecords = %d seconds" , maxChunkBufferRecords);
        msg(MSG_INFO, "  - maxFileCreationInterval = %d seconds" , maxFileCreationInterval);

	return 0;
}

