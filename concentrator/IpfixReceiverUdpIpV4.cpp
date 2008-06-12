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
 * @param ipAddr interface to use, if equals "", all interfaces will be used
 */
IpfixReceiverUdpIpV4::IpfixReceiverUdpIpV4(int port, std::string ipAddr)
	: statReceivedPackets(0)
{
	receiverPort = port;

	struct sockaddr_in serverAddress;

	listen_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if(listen_socket < 0) {
		perror("Could not create socket");
		THROWEXCEPTION("Cannot create IpfixReceiverUdpIpV4, socket creation failed");
	}
	
	// if ipAddr set: listen on a specific interface 
	// else: listen on all interfaces
	if(ipAddr == "")
		serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	else
		serverAddress.sin_addr.s_addr = inet_addr(ipAddr.c_str());
	
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(port);
	if(bind(listen_socket, (struct sockaddr*)&serverAddress, 
		sizeof(struct sockaddr_in)) < 0) {
		perror("Could not bind socket");
		THROWEXCEPTION("Cannot create IpfixReceiverUdpIpV4 %s:%d",ipAddr.c_str(), port );
	}

	SensorManager::getInstance().addSensor(this, "IpfixReceiverUdpIpV4", 0);

	msg(MSG_INFO, "UDP Receiver listening on %s:%d, FD=%d", (ipAddr == "")?std::string("ALL").c_str() : ipAddr.c_str(), 
								port, 
								listen_socket);
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
	
	struct timespec timeOut;
	fd_set nreadfds;

	/* set a 400ms time-out on the pselect */
	timeOut.tv_sec = 0L;
	timeOut.tv_nsec = 400000000L;
	
	while(!exitFlag) {
		int n;

		do {
			FD_ZERO(&nreadfds);
			FD_SET(listen_socket, &nreadfds);

			n = pselect(listen_socket + 1, &nreadfds, NULL, NULL, &timeOut, NULL);
			if (n < 0) {
				if (errno != EINTR)
					THROWEXCEPTION("select failed");
				
				printf ("n < 0\n");
				continue;
			}
			
			if (exitFlag)
				return;
		} while (n == 0);
		
		boost::shared_array<uint8_t> data(new uint8_t[MAX_MSG_LEN]);
		boost::shared_ptr<IpfixRecord::SourceID> sourceID(new IpfixRecord::SourceID);


		clientAddressLen = sizeof(struct sockaddr_in);
		n = recvfrom(listen_socket, data.get(), MAX_MSG_LEN,
			     0, (struct sockaddr*)&clientAddress, &clientAddressLen);
		if (n < 0) {
			msg(MSG_FATAL, "recvfrom returned without data, terminating listener thread");
			break;
		}
		
		if (isHostAuthorized(&clientAddress.sin_addr, sizeof(clientAddress.sin_addr))) {
			statReceivedPackets++;
// 			uint32_t ip = clientAddress.sin_addr.s_addr;
			memcpy(sourceID->exporterAddress.ip, &clientAddress.sin_addr.s_addr, 4);
			sourceID->exporterAddress.len = 4;
			sourceID->exporterPort = ntohs(clientAddress.sin_port);
			sourceID->protocol = IPFIX_protocolIdentifier_UDP;
			sourceID->receiverPort = receiverPort;
			sourceID->fileDescriptor = listen_socket;
			pthread_mutex_lock(&mutex);
			for (std::list<IpfixPacketProcessor*>::iterator i = packetProcessors.begin(); i != packetProcessors.end(); ++i) { 
				(*i)->processPacket(data, n, sourceID);
			}
		} else {
			msg(MSG_FATAL, "packet from unauthorized host %s discarded", inet_ntoa(clientAddress.sin_addr));
		}
	}
}

/**
 * statistics function called by StatisticsManager
 */
std::string IpfixReceiverUdpIpV4::getStatisticsXML(double interval)
{
	ostringstream oss;
	
	oss << "<receivedPackets>" << statReceivedPackets << "</receivedPackets>" << endl;	

	return oss.str();
}
