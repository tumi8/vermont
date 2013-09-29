/*
 * IPFIX Concentrator Module Library
 * Copyright (C) 2009 Tobias Limmer <http://www7.informatik.uni-erlangen.de/~limmer>
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


#include "IpfixFileWriter.hpp"

#include "common/ipfixlolib/ipfixlolib.h"
#include "common/ipfixlolib/ipfix.h"
#include "common/msg.h"

#include <stdexcept>
#include <string.h>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>


/* go back to SENDER_TEMPLATE_ID_LOW if _HI is reached */
#define SENDER_TEMPLATE_ID_HI 60000

#define MAX_RECORD_RATE 33554432

/**
 * Creates a new IPFIXFileWriter. Do not forget to call @c startIpfixFileWriter() to begin sending
 */
IpfixFileWriter::IpfixFileWriter(uint16_t observationDomainId, std::string filenamePrefix, 
	std::string destinationPath, uint32_t maximumFilesize)
			: IpfixSender(observationDomainId, MAX_RECORD_RATE)
{
	// check if directory base exists
	if (!boost::filesystem::is_directory(destinationPath)) {
		THROWEXCEPTION("Directory %s does not exists or is not a directory!", destinationPath.c_str());
	}

	if (filenamePrefix != "") {
		if(addCollector(observationDomainId, filenamePrefix, destinationPath, maximumFilesize) != 0) {
			THROWEXCEPTION("IpfixFileWriter's Collector addition failed");
			return;
		}
	} else {
		THROWEXCEPTION("IpfixFileWriter: no filename prefix given. Prefix is required though!");
	}
	
	msg(MSG_DEBUG, "IpfixFileWriter: running");
}

IpfixFileWriter::~IpfixFileWriter() {
}

/**
 * Add another IPFIX collector to export the stream to
 * the lowlevel stuff in handled by underlying ipfixlolib
 */
int IpfixFileWriter::addCollector(uint16_t observationDomainId, std::string filenamePrefix, 
			std::string destinationPath, uint32_t maximumFilesize) 
{
	ipfix_exporter *ex = (ipfix_exporter *)ipfixExporter;
	
	if(destinationPath.at(destinationPath.length()-1) != '/') 
		destinationPath += "/";
	std::string my_filename = destinationPath + filenamePrefix; 
	if (maximumFilesize <= 0) maximumFilesize = DEFAULTFILESIZE;
	if(maximumFilesize < 64)
		 msg(MSG_ERROR, 
		   "maximum filsize < maximum message length - this could lead to serious problems");

	if(ipfix_add_collector(ex, my_filename.c_str(), maximumFilesize, DATAFILE, NULL) != 0) {
		msg(MSG_FATAL, "IpfixFileWriter: ipfix_add_collector of %s failed", my_filename.c_str());
		return -1;
	}

	msg(MSG_INFO, "IpfixFileWriter: adding %s to exporter", my_filename.c_str());
	msg(MSG_INFO, "IpfixFileWriter initialized with the following parameters");
	msg(MSG_INFO, "  - Basename = %s", my_filename.c_str());
	msg(MSG_INFO, "  - maximumFilesize = %d KiB" , maximumFilesize);

	return 0;
}

