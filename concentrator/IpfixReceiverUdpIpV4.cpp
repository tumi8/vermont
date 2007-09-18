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

#include "IpfixReceiverUdpIpV4.hpp"

#include "IpfixPacketProcessor.hpp"
#include "IpfixParser.hpp"
#include "ipfix.hpp"
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
#include <sstream>


using namespace std;

/** 
 * Does UDP/IPv4 specific initialization.
 * @param port Port to listen on
 */
IpfixReceiverUdpIpV4::IpfixReceiverUdpIpV4(int port) 
	: statReceivedPackets(0)
{
	struct sockaddr_in serverAddress;

	listen_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if(listen_socket < 0) {
		perror("Could not create socket");
		THROWEXCEPTION("Cannot create IpfixReceiverUdpIpV4");
	}
	
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddress.sin_port = htons(port);
	if(bind(listen_socket, (struct sockaddr*)&serverAddress, 
		sizeof(struct sockaddr_in)) < 0) {
		perror("Could not bind socket");
		THROWEXCEPTION("Cannot create IpfixReceiverUdpIpV4");
	}

	StatisticsManager::getInstance().addModule(this);
}


/**
 * Does UDP/IPv4 specific cleanup
 */
IpfixReceiverUdpIpV4::~IpfixReceiverUdpIpV4() {
	close(listen_socket);
}


/**
 * UDP specific listener function. This function is called by @c listenerThread()
 */
void IpfixReceiverUdpIpV4::run() {
	struct sockaddr_in clientAddress;
	socklen_t clientAddressLen;
	
	while(!exitFlag) {
		boost::shared_array<uint8_t> data(new uint8_t[MAX_MSG_LEN]);
		boost::shared_ptr<IpfixRecord::SourceID> sourceID(new IpfixRecord::SourceID);
		int n;

		clientAddressLen = sizeof(struct sockaddr_in);
		// FIXME: recvfrom is blocking - this function does not exit when no packet arrives
		n = recvfrom(listen_socket, data.get(), MAX_MSG_LEN,
			     0, (struct sockaddr*)&clientAddress, &clientAddressLen);
		if (n < 0) {
			msg(MSG_DEBUG, "recvfrom returned without data, terminating listener thread");
			break;
		}
		
		if (isHostAuthorized(&clientAddress.sin_addr, sizeof(clientAddress.sin_addr))) {

			statReceivedPackets++;
			uint32_t ip = ntohl(clientAddress.sin_addr.s_addr);
			memcpy(sourceID->exporterAddress.ip, &ip, 4);
			sourceID->exporterAddress.len = 4;

			for (std::list<IpfixPacketProcessor*>::iterator i = packetProcessors.begin(); i != packetProcessors.end(); ++i) { 
				(*i)->processPacket(data, n, sourceID);
			}
		}
		else{
			msg(MSG_DEBUG, "packet from unauthorized host %s discarded", inet_ntoa(clientAddress.sin_addr));
		}
	}
}

/**
 * statistics function called by StatisticsManager
 */
std::string IpfixReceiverUdpIpV4::getStatistics()
{
	ostringstream oss;
	
	uint32_t recv = statReceivedPackets;
	statReceivedPackets -= recv;
	
	oss << "IpfixReceiverUdpIpV4: received packets: " << recv << endl;	

	return oss.str();
}
