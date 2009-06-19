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

IpfixReceiverFile::IpfixReceiverFile(std::string packetFileBasename, std::string packetFileDirectory,
		int c_from, int c_to)	: 
		packet_file_basename(packetFileBasename),
		packet_file_directory(packetFileDirectory),
		from(c_from), to(c_to)
{
	if (packet_file_directory.at(packet_file_directory.length()-1) != '/')
				packet_file_directory += "/";
	
	//get the last filenumber, if not specified by the user
	if (to<0){
		int _max = 0;
		boost::filesystem::path full_path =
			boost::filesystem::system_complete(boost::filesystem::path(packet_file_directory, boost::filesystem::native));

		if (!boost::filesystem::exists(full_path)) THROWEXCEPTION("Packet file directory does not exist");
		if (!boost::filesystem::is_directory(full_path)) THROWEXCEPTION("Packet file directory is not a directory");

		boost::filesystem::directory_iterator dir_iterator = boost::filesystem::directory_iterator(full_path);
		boost::filesystem::directory_iterator end_itr;
		size_t strlen = packetFileBasename.length();
		while (dir_iterator != end_itr) {
			if (dir_iterator->leaf().length() == strlen+10 && 
					dir_iterator->leaf().substr(0,strlen) == packetFileBasename){
				int _filenum = boost::lexical_cast<int>(dir_iterator->leaf().substr(strlen, 10));
				if (_filenum > _max) _max = _filenum;
			}
			dir_iterator++;
		}
		to = _max;
	}
	msg(MSG_INFO, "IpfixReceiverFile initialized with the following parameters:");
	msg(MSG_INFO, "  - packet_file_directory = %s", packet_file_directory.c_str());
	msg(MSG_INFO, "  - packet_file_basename = %s", packet_file_basename.c_str());
	msg(MSG_INFO, "  - Start (from) = %d" , from);
	msg(MSG_INFO, "  - End (to) = %d" , to);
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

	for(int filecount=from; filecount<=to; filecount++){
		ostringstream numberformat (ostringstream::out);
		numberformat.width(10);
		numberformat.fill('0');
		numberformat << filecount;
		std::string packet_file_path = packet_file_directory + packet_file_basename + numberformat.str();

		msg(MSG_DEBUG, "IpfixReceiverFile: Trying to read packet from file \"%s\"", packet_file_path.c_str());
		std::ifstream packetFile(packet_file_path.c_str(), std::ios::in | std::ios::binary);
		if (packetFile.fail()){
			msg(MSG_FATAL, "Couldn't open inputfile %s\n", packet_file_path.c_str());
			continue;
		}

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
				msg(MSG_ERROR, "IpfixReceiverFile: packet at idx=%u too big with n=%u in file \"%s\"", 
						idx, n, packet_file_path.c_str());
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

		msg(MSG_INFO, "IpfixReceiverFile: File %s ended after %u bytes.", packet_file_path.c_str(), idx);
	}
	if (vmodule) {
		vmodule->shutdownVermont();
	} else {
		msg(MSG_ERROR, "IpfixReceiverFile: failed to shut down Vermont, internal error!");
	}
}

