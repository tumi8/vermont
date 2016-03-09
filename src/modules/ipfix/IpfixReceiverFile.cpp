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
#include "common/ipfixlolib/ipfix.h"
#include "common/msg.h"

#include <stdexcept>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#define MAXMISSINGFILES 10


bool IpfixReceiverFile::checkint(const char *my_string){
	size_t stringlength = strlen(my_string);
	for (size_t j=0; j<stringlength; j++)
		if((int)my_string[j] < 0x30 || (int)my_string[j] > 0x39) 
				return false;

	return true;
}

IpfixReceiverFile::IpfixReceiverFile(std::string packetFileBasename, 
	std::string packetFileDirectory, int c_from, int c_to, bool ignore,
	 float offlinespeed)	: 
		packet_file_directory(packetFileDirectory),
		packet_file_basename(packetFileBasename),
		from(c_from), to(c_to), ignore_timestamps(ignore),
		stretchTime(offlinespeed), stretchTimeInt(1)
{
	if (packet_file_directory.at(packet_file_directory.length()-1) != '/')
				packet_file_directory += "/";

	if(stretchTime <=0.0 ) ignore_timestamps = true;

	//adopted from Observer
	if (stretchTime == 1.0) ; // do nothing
	else {
		float m = stretchTime;
		stretchTime = 1.0/m;
		if(m < 1.0){
			stretchTimeInt = (uint16_t)(1/m);
			if((1-stretchTimeInt*m) > 0.1)
				stretchTimeInt = 0; //use float
			else
				msg(MSG_INFO, "IpfixReceiverFile: speed multiplier set to %f "
					"in order to allow integer multiplication", 1.0/stretchTimeInt);
		}
		else 
			stretchTimeInt = 0;
	}
	//get the last filenumber, if not specified by the user
	if (to<0){
		int maxnum = 0;
		boost::filesystem::path full_path =
#if BOOST_FILESYSTEM_VERSION == 3 
			boost::filesystem::system_complete(boost::filesystem::path(packet_file_directory));
#elif BOOST_FILESYSTEM_VERSION == 2 
			boost::filesystem::system_complete(boost::filesystem::path(packet_file_directory,
				 boost::filesystem::native));
#else
			boost::filesystem::system_complete(boost::filesystem::path(packet_file_directory));
#endif 

		if (!boost::filesystem::exists(full_path)) 
			THROWEXCEPTION("Packet file directory does not exist");
		if (!boost::filesystem::is_directory(full_path)) 
			THROWEXCEPTION("Packet file directory is not a directory");

		boost::filesystem::directory_iterator dir_iterator = 
					boost::filesystem::directory_iterator(full_path);
		boost::filesystem::directory_iterator end_itr;
		size_t strlen = packetFileBasename.length();
		while (dir_iterator != end_itr) {
#if BOOST_FILESYSTEM_VERSION == 3
			if (dir_iterator->path().filename().string().length() == strlen+10 && 
					dir_iterator->path().filename().string().substr(0,strlen) == packetFileBasename
						&& checkint(dir_iterator->path().filename().string().substr(strlen,10).c_str()))
#elif BOOST_FILESYSTEM_VERSION == 2
			if (dir_iterator->leaf().length() == strlen+10 && 
					dir_iterator->leaf().substr(0,strlen) == packetFileBasename
						&& checkint(dir_iterator->leaf().substr(strlen,10).c_str()))
#else
			if (dir_iterator->path().filename().length() == strlen+10 && 
					dir_iterator->path().filename().substr(0,strlen) == packetFileBasename
						&& checkint(dir_iterator->path().filename().substr(strlen,10).c_str()))

#endif
			{
#if BOOST_FILESYSTEM_VERSION == 3
				int filenum = boost::lexical_cast<int>(dir_iterator->path().filename().string().substr(strlen, 10));
#elif BOOST_FILESYSTEM_VERSION == 2
				int filenum = boost::lexical_cast<int>(dir_iterator->leaf().substr(strlen, 10));
#else
				int filenum = boost::lexical_cast<int>(dir_iterator->path().filename().substr(strlen, 10));
				
#endif
				if (filenum > maxnum) maxnum = filenum;
			}
			dir_iterator++;
		}
		to = maxnum;
	}
	msg(MSG_INFO, "IpfixReceiverFile initialized with the following parameters:");
	msg(MSG_INFO, "  - packet_file_directory = %s", packet_file_directory.c_str());
	msg(MSG_INFO, "  - packet_file_basename = %s", packet_file_basename.c_str());
	msg(MSG_INFO, "  - Start (from) = %d" , from);
	msg(MSG_INFO, "  - End (to) = %d" , to);
	msg(MSG_INFO, "  - ignoreTimestamps = %s" , (ignore_timestamps) ? "true" : "false");
	if(! ignore_timestamps) msg(MSG_INFO, "  - stretchTime = %f", stretchTime);
}


IpfixReceiverFile::~IpfixReceiverFile()
{
	if (packetFile.is_open()) packetFile.close();
}


/**
 * specific listener function. This function is called by @c listenerThread()
 */
void IpfixReceiverFile::run()
{
	boost::shared_array<uint8_t> data;
	boost::shared_ptr<IpfixRecord::SourceID> sourceID(new IpfixRecord::SourceID);
	struct timeval msg_first, real_start, msg_now, real_now, 
		   msg_delta, real_delta, sleep_time, tmp_delta;
	struct timespec wait_spec;
	bool first = true;
	int missing = 0; 

	settimezero(&msg_first);
	settimezero(&real_delta);
	settimezero(&real_start);
	settimezero(&msg_delta);
	settimezero(&msg_now);
	settimezero(&tmp_delta);
	
	for(int filecount=from; filecount<=to && !exitFlag; filecount++){
		ostringstream numberformat (ostringstream::out);
		numberformat.width(10);
		numberformat.fill('0');
		numberformat << filecount;
		std::string packet_file_path = packet_file_directory + packet_file_basename
										 + numberformat.str();

		msg(MSG_DEBUG, "IpfixReceiverFile: Trying to read message from file \"%s\"", 
			packet_file_path.c_str());

		packetFile.open(packet_file_path.c_str(), std::ios::in | std::ios::binary);
		if (packetFile.fail()){
			msg(MSG_FATAL, "Couldn't open inputfile %s", packet_file_path.c_str());
			if (++missing > MAXMISSINGFILES){
				msg(MSG_FATAL, "Couldn't open %d files in a row...terminating", MAXMISSINGFILES);
				break;
			}
			continue;
		}
		missing = 0;

		packetFile.seekg(0, std::ios::end);
		uint64_t end = packetFile.tellg();
		packetFile.seekg(0, std::ios::beg);

		uint64_t idx = 0;
		while (idx<end && !exitFlag) {
			uint16_t n;
			packetFile.seekg(sizeof(uint16_t), std::ios::cur); /*version number offset*/
			packetFile.read(reinterpret_cast<char*>(&n), sizeof(uint16_t));
			n = ntohs(n);
			/*reset the filepointer to the begin of the message*/
			packetFile.seekg(-(int)(sizeof(uint32_t)), std::ios::cur); 

			/** @todo uint_16 can never be > 65536
			if (n > MAX_MSG_LEN) {
				msg(MSG_ERROR, "IpfixReceiverFile: packet at idx=%u too big with n=%u in file \"%s\"", 
						idx, n, packet_file_path.c_str());
				continue;
			}
*/

			data.reset(new uint8_t[MAX_MSG_LEN]);
			packetFile.read(reinterpret_cast<char*>(data.get()), n);
			idx += n;

			if (packetFile.bad())  {
				msg(MSG_ERROR, "IpfixReceiverFile: bad packet_file: %s", packet_file_path.c_str());
				continue;
			}

			// FIXME: The received ip address will be 1.0.0.127
			uint32_t ip = 0x7F000001; // 127.0.0.1
			memcpy(sourceID->exporterAddress.ip, &ip, 4);
			sourceID->exporterAddress.len = 4;

			if (! ignore_timestamps){
				uint32_t exporttime = (uint32_t) 
						((uint32_t)(0xff & data[4])<<24 | (uint32_t)(0xff & data[5])<<16 | 
						(uint32_t)(0xff & data[6])<<8 | (uint32_t)(0xff & data[7]));

				if(gettimeofday(&real_now, NULL) != 0){
					msg(MSG_FATAL, "Error gettimeofday: %s", strerror(errno));
					msg(MSG_FATAL, "Ignoring timestamps!");
					ignore_timestamps = true;
					first = true; 
				}
				if (first){
					first = false;
					msg_first.tv_sec = (time_t)exporttime;
					real_start.tv_sec = real_now.tv_sec;
					real_start.tv_usec = real_now.tv_usec;
				}
				else{
					msg_now.tv_sec = (time_t)exporttime;
					msg(MSG_DEBUG, "Exporttime: %u", exporttime);
					//msg_delta.tv_sec = msg_now.tv_sec - msg_first.tv_sec;
					timersub(&msg_now, &msg_first, &msg_delta);
					//real_delta.tv_sec = real_now.tv_sec - real_start.tv_sec;
					timersub(&real_now, &real_start, &real_delta);
					
					if(stretchTimeInt != 1){
						if(stretchTimeInt == 0)
							timermulfloat(&msg_delta, &tmp_delta, stretchTime);
						else
							timermul(&msg_delta, &tmp_delta, stretchTimeInt);
					}
					else{
						tmp_delta.tv_sec = msg_delta.tv_sec;
						tmp_delta.tv_usec = msg_delta.tv_usec;
					}

					//if(real_delta.tv_sec < msg_delta.tv_sec) sleep(sleep_time.tv_sec);
					if(timercmp(&real_delta, &tmp_delta, <)){
						//sleep_time.tv_sec = msg_delta.tv_sec - real_delta.tv_sec;
						timersub(&tmp_delta, &real_delta, &sleep_time);

						msg(MSG_DEBUG, "msg_delta: %06us %06uus | tmp_delta: %06us %06uus | "
						"real_delta: %06us %06uus | sleep_time: %06us %06uus",
						  	(uint32_t) msg_delta.tv_sec, (uint32_t) msg_delta.tv_usec,
						  	(uint32_t) tmp_delta.tv_sec, (uint32_t) tmp_delta.tv_usec,
							(uint32_t) real_delta.tv_sec, (uint32_t) real_delta.tv_usec, 
							(uint32_t) sleep_time.tv_sec,(uint32_t) sleep_time.tv_usec);

						wait_spec.tv_sec = sleep_time.tv_sec;
						wait_spec.tv_nsec = sleep_time.tv_usec*1000;
						msg(MSG_DEBUG, "sleeping for: %06us %06uus",
							 (uint32_t)sleep_time.tv_sec, (uint32_t)sleep_time.tv_usec);
						if(nanosleep(&wait_spec, NULL)){
							msg(MSG_ERROR, "nanosleep returned non-zero value: %s", strerror(errno));
						}
					}
					else{
						msg(MSG_DEBUG, "Not sleeping");
					}
				}
			}

			for (std::list<IpfixPacketProcessor*>::iterator i = packetProcessors.begin(); 
					i != packetProcessors.end(); ++i) {
				DPRINTF("Data block starts with: %x %x %x %x", data[0], data[1], data[2], data[3]);
				(*i)->processPacket(data, n, sourceID);
			}
		}
		packetFile.close();

		msg(MSG_INFO, "IpfixReceiverFile: File %s ended after %llu bytes.", 
			packet_file_path.c_str(), idx);
	}
	msg(MSG_DEBUG, "real_start: %lu  msg_start: %lu  real_now: %lu  msg_now: %lu", 
		real_start.tv_sec, msg_first.tv_sec, real_now.tv_sec, msg_now.tv_sec);
	if (vmodule) {
		vmodule->shutdownVermont();
	} else {
		msg(MSG_ERROR, "IpfixReceiverFile: failed to shut down Vermont, internal error!");
	}
}

