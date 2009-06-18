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

#include "IpfixReceiverFile.hpp"

#include "IpfixPacketProcessor.hpp"
#include "IpfixParser.hpp"
#include "common/ipfixlolib/ipfix.h"
#include "common/msg.h"

#include <stdexcept>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fstream>

IpfixReceiverFile::IpfixReceiverFile(std::string packet_file_path)
	: packet_file_path(packet_file_path)
{
	/*fh = open(packet_file_path, O_LARGEFILE|O_RDONLY);
	if (fh==0) {
		THROWEXCEPTION("IpfixFileReader: failed to open file %s, error: %s", packet_file_path.c_str(), strerror(errno));
	}*/
}


IpfixReceiverFile::~IpfixReceiverFile()
{
	if (fh>0) close(fh);
}


/**
 * specific listener function. This function is called by @c listenerThread()
 */
void IpfixReceiverFile::run()
{
	boost::shared_array<uint8_t> data;
	boost::shared_ptr<IpfixRecord::SourceID> sourceID(new IpfixRecord::SourceID);

	msg(MSG_DEBUG, "IpfixReceiverFile: Trying to read packet from file \"%s\"", packet_file_path.c_str());
	std::ifstream packetFile(packet_file_path.c_str(), std::ios::in | std::ios::binary);

	packetFile.seekg(0, std::ios::end);
	uint32_t end = packetFile.tellg();
	packetFile.seekg(0, std::ios::beg);

	uint32_t idx = 0;
	while (idx<end) {
		uint16_t n;
		packetFile.seekg(sizeof(uint16_t), std::ios::cur); /*version numebr offset*/
		packetFile.read(reinterpret_cast<char*>(&n), sizeof(uint16_t));
		n = ntohs(n);
		packetFile.seekg( -(int)sizeof(uint32_t), std::ios::cur); /*reset the filepointer to the begin of the message*/

		//msg(MSG_DIALOG, "n=%u", n);

		if (n > MAX_MSG_LEN) {
			msg(MSG_ERROR, "IpfixReceiverFile: packet at idx=%u too big with n=%u in file \"%s\"", idx, n, packet_file_path.c_str());
			continue;
		}

		data.reset(new uint8_t[MAX_MSG_LEN]);
		packetFile.read(reinterpret_cast<char*>(data.get()), n);
		idx += n;

		if (packetFile.bad())  {
			msg(MSG_DEBUG, "IpfixReceiverFile: could not read from packet file, terminating listener thread");
			break;
		}

		uint32_t ip = 0x7F000001; // 127.0.0.1
		memcpy(sourceID->exporterAddress.ip, &ip, 4);
		sourceID->exporterAddress.len = 4;

		for (std::list<IpfixPacketProcessor*>::iterator i = packetProcessors.begin(); i != packetProcessors.end(); ++i) {
			DPRINTF("Data block starts with: %x %x %x %x", data[0], data[1], data[2], data[3]);
			(*i)->processPacket(data, n, sourceID);
		}
	}

	packetFile.close();

	msg(MSG_DIALOG, "IpfixReceiverFile: File ended after %u bytes.", idx);
	if (vmodule) {
		vmodule->shutdownVermont();
	} else {
		msg(MSG_ERROR, "IpfixReceiverFile: failed to shut down Vermont, internal error!");
	}
}

