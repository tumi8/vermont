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
#include <sstream>


using namespace std;

/** 
 * Does UDP/IPv4 specific initialization.
 * @param port Port to listen on
 * @param ipAddr interface to use, if equals "", all interfaces will be used
 */
IpfixReceiverUdpIpV4::IpfixReceiverUdpIpV4(int port, std::string ipAddr, const uint32_t buffer)
	: statReceivedPackets(0)
{
	receiverPort = port;

	struct sockaddr_in serverAddress;

	listen_socket = socket(AF_INET, SOCK_DGRAM, 0);
	if(listen_socket < 0) {
		/* ASK: error should be written to log file */
		perror("Could not create socket");
		/* ASK: Why not throw? printf format */
		THROWEXCEPTION("Cannot create IpfixReceiverUdpIpV4, socket creation failed");
	}

	setBufferSize(listen_socket, buffer);
	
	// if ipAddr set: listen on a specific interface 
	// else: listen on all interfaces
	if(ipAddr == "")
		serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	else
		/* ASK: check return value. inet_addr() is obsolete. inet_aton should be used. */
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
	SensorManager::getInstance().removeSensor(this);
}


/**
 * UDP specific listener function. This function is called by @c listenerThread()
 */
void IpfixReceiverUdpIpV4::run() {
	struct sockaddr_in clientAddress;
	socklen_t clientAddressLen;
	clientAddressLen = sizeof(struct sockaddr_in);
	
	fd_set fd_array; //all active filedescriptors
	fd_set readfds;  //parameter for for pselect

	int ret;
	struct timespec timeOut;

	FD_ZERO(&fd_array);
	FD_SET(listen_socket, &fd_array);

	/* set a 400ms time-out on the pselect */
	timeOut.tv_sec = 0L;
	timeOut.tv_nsec = 400000000L;
	
	while(!exitFlag) {
		readfds = fd_array; // because select() changes readfds
		ret = pselect(listen_socket + 1, &readfds, NULL, NULL, &timeOut, NULL);
		if (ret == 0) {
			/* Timeout */
			continue;
    		}
		if ((ret == -1) && (errno == EINTR)) {
			/* There was a signal... ignore */
			continue;
    		}
    		if (ret < 0) {
    			msg(MSG_ERROR ,"select() returned with an error");
			THROWEXCEPTION("IpfixReceiverUdpIpV4: terminating listener thread");
			break;
		}

		boost::shared_array<uint8_t> data(new uint8_t[MAX_MSG_LEN]);

		ret = recvfrom(listen_socket, data.get(), MAX_MSG_LEN,
			     0, (struct sockaddr*)&clientAddress, &clientAddressLen);
		if (ret < 0) {
			msg(MSG_FATAL, "recvfrom returned without data, terminating listener thread");
			break;
		}
		
		if (isHostAuthorized(&clientAddress.sin_addr, sizeof(clientAddress.sin_addr))) {
			statReceivedPackets++;
// 			uint32_t ip = clientAddress.sin_addr.s_addr;
			boost::shared_ptr<IpfixRecord::SourceID> sourceID(new IpfixRecord::SourceID);
			memcpy(sourceID->exporterAddress.ip, &clientAddress.sin_addr.s_addr, 4);
			sourceID->exporterAddress.len = 4;
			sourceID->exporterPort = ntohs(clientAddress.sin_port);
			sourceID->protocol = IPFIX_protocolIdentifier_UDP;
			sourceID->receiverPort = receiverPort;
			sourceID->fileDescriptor = listen_socket;
			mutex.lock();
			for (std::list<IpfixPacketProcessor*>::iterator i = packetProcessors.begin(); i != packetProcessors.end(); ++i) { 
				(*i)->processPacket(data, ret, sourceID);
			}
			mutex.unlock();
		} else {
			msg(MSG_VDEBUG, "IpfixReceiverUdpIpv4: packet from unauthorized host %s discarded", inet_ntoa(clientAddress.sin_addr));
		}
	}
	msg(MSG_DEBUG, "IpfixReceiverUdpIpV4: Exiting");
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
