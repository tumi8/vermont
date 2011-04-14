/*
 * IPFIX Concentrator Module Library
 * Copyright (C) 2004 Christoph Sommer <http://www.deltadevelopment.de/users/christoph/ipfix/>
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

#include "common/ipfixlolib/ipfixlolib.h"
#ifdef IPFIXLOLIB_RAWDIR_SUPPORT

#include <stdexcept>
#include <string.h>
#include "IpfixRawdirWriter.hpp"
#include "common/ipfixlolib/ipfix.h"

#include "common/msg.h"

/* go back to SENDER_TEMPLATE_ID_LOW if _HI is reached */
#define SENDER_TEMPLATE_ID_HI 60000

/**
 * Creates a new IPFIX Exporter. Do not forget to call @c startIpfixRawdirWriter() to begin sending
 * @param sourceID Source ID this exporter will report
 * @param ip destination collector's address
 * @param port destination collector's port
 * @return handle to use when calling @c destroyIpfixRawdirWriter()
 */
IpfixRawdirWriter::IpfixRawdirWriter(uint32_t observationDomainId, std::string packetDirectoryName) 
	: IpfixSender(observationDomainId, IS_DEFAULT_MAXRECORDRATE) {

	if (packetDirectoryName != "") {
		if(addCollector(packetDirectoryName) != 0) {
			THROWEXCEPTION("IpfixRawdirWriter's Collector addition failed");
			return;	
		}
	}

	msg(MSG_DEBUG, "IpfixRawdirWriter: running");
}

IpfixRawdirWriter::~IpfixRawdirWriter() {
}

/**
 * Add another IPFIX collector to export the stream to
 * the lowlevel stuff in handled by underlying ipfixlolib
 * @param packetDirectoryName path to create raw packet files in
 */
int IpfixRawdirWriter::addCollector(std::string packetDirectoryName) {
	ipfix_exporter *ex = (ipfix_exporter *)ipfixExporter;

	if(ipfix_add_collector(ex, packetDirectoryName.c_str(), 0, RAWDIR, NULL) != 0) {
		msg(MSG_FATAL, "IpfixRawdirWriter: ipfix_add_collector of %s failed", packetDirectoryName.c_str());
		return -1;
	}
	
	msg(MSG_INFO, "IpfixRawdirWriter: adding %s to exporter", packetDirectoryName.c_str());

	return 0;
}

#endif //IPFIXLOLIB_RAWDIR_SUPPORT

