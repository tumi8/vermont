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

#ifdef HAVE_BOOST_FILESYSTEM

#include "IpfixRawdirReader.hpp"

#include "IpfixPacketProcessor.hpp"
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

IpfixRawdirReader::IpfixRawdirReader(std::string packet_directory_path) : packet_directory_path(packet_directory_path) {
	boost::filesystem::path //full_path(boost::filesystem::initial_path<boost::filesystem::path>());
#if BOOST_FILESYSTEM_VERSION == 3
	full_path = boost::filesystem::system_complete(boost::filesystem::path(packet_directory_path));
#else
	full_path = boost::filesystem::system_complete(boost::filesystem::path(packet_directory_path, boost::filesystem::native));
#endif

	if (!boost::filesystem::exists(full_path)) THROWEXCEPTION("Packet directory path does not exist");
	if (!boost::filesystem::is_directory(full_path)) THROWEXCEPTION("Packet directory path is not a directory");

	dir_iterator = boost::filesystem::directory_iterator(full_path);
}


IpfixRawdirReader::~IpfixRawdirReader() {
}


/**
 * specific listener function. This function is called by @c listenerThread()
 */
void IpfixRawdirReader::run() {
	boost::shared_array<uint8_t> data;
	boost::shared_ptr<IpfixRecord::SourceID> sourceID(new IpfixRecord::SourceID);
	
	while(!exitFlag) {

		if (dir_iterator == end_iterator) {
			msg(MSG_DEBUG, "No more packets in packet directory path, terminating listener thread");
			break;
		}

#if BOOST_FILESYSTEM_VERSION == 3
		std::string fname = packet_directory_path+"/"+dir_iterator->path().filename().string();
#elif BOOST_FILESYSTEM_VERSION == 2
		std::string fname = packet_directory_path+"/"+dir_iterator->leaf();
#else
		std::string fname = packet_directory_path+"/"+dir_iterator->path().filename();
#endif
		if (boost::filesystem::is_directory(*dir_iterator)) {
			msg(MSG_DEBUG, "Skipping directory \"%s\"", fname.c_str());
			dir_iterator++;
			continue;
		}

		dir_iterator++;

		msg(MSG_DEBUG, "Trying to read packet from file \"%s\"", fname.c_str());
		std::ifstream packetFile(fname.c_str(), std::ios::in | std::ios::binary);

		packetFile.seekg(0, std::ios::end);
		int n = packetFile.tellg();
		packetFile.seekg(0, std::ios::beg);

		if (n > MAX_MSG_LEN) {
			msg(MSG_DEBUG, "File too big \"%s\"", fname.c_str());
			continue;
		}

		data.reset(new uint8_t[MAX_MSG_LEN]);
		packetFile.read(reinterpret_cast<char*>(data.get()), n);

		if (packetFile.bad())  {
			msg(MSG_DEBUG, "could not read from packet file, terminating listener thread");
			break;
		}

		packetFile.close();

		uint32_t ip = 0x7F000001; // 127.0.0.1
		memcpy(sourceID->exporterAddress.ip, &ip, 4);
		sourceID->exporterAddress.len = 4;

		for (std::list<IpfixPacketProcessor*>::iterator i = packetProcessors.begin(); i != packetProcessors.end(); ++i) { 
		msg(MSG_DEBUG, "Data block starts with: %x %x %x %x", data[0], data[1], data[2], data[3]);
			(*i)->processPacket(data, n, sourceID);
		}

		//sleep(1);
	}
	
}

#endif //HAVE_BOOST_FILESYSTEM

