/*
 * IPFIX Concentrator Module Library - SCTP Receiver
 * Copyright (C) 2007 Alex Melnik
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

#ifdef SUPPORT_SCTP
#include "common/ipfixlolib/ipfixlolib.h"

#include "IpfixReceiverSctp.hpp"

#include "IpfixPacketProcessor.hpp"
#include "common/ipfixlolib/ipfix.h"
#include "common/msg.h"

#include <stdexcept>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <map>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

/** 
 * Does SCTP/IPv4 specific initialization.
 * @param port Port to listen on
 */
IpfixReceiverSctp::IpfixReceiverSctp(int port, std::string ipAddr) {
	receiverPort = port;

	bool listen4 = false;
	bool listen6 = false;
	if (ipAddr == "") {
		createIPv4Socket(ipAddr, SOCK_STREAM, IPPROTO_SCTP, port);
		createIPv6Socket(ipAddr, SOCK_STREAM, IPPROTO_SCTP, port);
		listen4 = listen6 = true;
	} else {
		enum receiver_address_type addrType;
		addrType = getAddressType(ipAddr);
		if (addrType == IPv4_ADDRESS) {
			createIPv4Socket(ipAddr, SOCK_STREAM, IPPROTO_SCTP, port);
			listen4 = true;
		} else if (addrType == IPv6_ADDRESS) {
			createIPv6Socket(ipAddr, SOCK_STREAM, IPPROTO_SCTP, port);
			listen6 = true;
		} else {
			THROWEXCEPTION("Protocol for Collector \"%s\" not supported", ipAddr.c_str());
		}
	}

	if (listen4) {
		if(listen(socket4, SCTP_MAX_BACKLOG) < 0 ) {
			msg(MSG_ERROR ,"Could not listen on SCTP/IPv4 socket %i", socket4);
			THROWEXCEPTION("Cannot create IpfixReceiverSctp");
		}
	}
	if (listen6) {
		if(listen(socket6, SCTP_MAX_BACKLOG) < 0 ) {
			msg(MSG_ERROR ,"Could not listen on SCTP/IPv6 socket %i", socket6);
			THROWEXCEPTION("Cannot create IpfixReceiverSctp");
		}
	}

	msg(MSG_INFO, "SCTP Receiver listening on %s:%d, FDv4=%d FDv6=%d", (ipAddr == "")?std::string("ALL").c_str() : ipAddr.c_str(), 
								port, 
								socket4, socket6);
	return;
}


/**
 * Does SCTP/IPv4 specific cleanup
 */
IpfixReceiverSctp::~IpfixReceiverSctp() {
}


/**
 * SCTP specific listener function. This function is called by @c listenerThread()
 */
void IpfixReceiverSctp::run() 
{
	struct sockaddr_in clientAddress4;
	struct sockaddr_in6 clientAddress6;
	socklen_t clientAddressLen4, clientAddressLen6;
	clientAddressLen4 = sizeof(struct sockaddr_in);
	clientAddressLen6 = sizeof(struct sockaddr_in6);
	
	std::map<int, int> fd_layer3_map;

	fd_set fd_array; //all active filedescriptors
	fd_set readfds;  //parameter for for pselect
	int maxfd;
	int firstNoneControl; // stores the maximum of socket4, socket6 as we need it as base for looping over the accepted sockets	

	int ret;
	int rfd;
	struct timespec timeOut;
	
	FD_ZERO(&fd_array);
	if (socket4 != -1)
		FD_SET(socket4, &fd_array); // add listensocket
	if (socket6 != -1)
		FD_SET(socket6, &fd_array); // add listensocket
	maxfd = std::max(socket4, socket6);
	firstNoneControl = maxfd + 1;
	
	/* set a 400ms time-out on the pselect */
	timeOut.tv_sec = 0L;
	timeOut.tv_nsec = 400000000L;
	
	while(!exitFlag) {
		readfds = fd_array; // because select() changes readfds
		ret = pselect(maxfd + 1, &readfds, NULL, NULL, &timeOut, NULL); // check only for something to read
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
			THROWEXCEPTION("IpfixReceiverSctp: terminating listener thread");
			break;
		}
		// looking for a new client to connect at listen_socket
		if (FD_ISSET(socket4, &readfds)){
			rfd = accept(socket4, (struct sockaddr*)&clientAddress4, &clientAddressLen4);
			
			if (rfd >= 0){
				if (isHostAuthorized(&clientAddress4.sin_addr, sizeof(clientAddress4.sin_addr))) {
					FD_SET(rfd, &fd_array); // add new client to fd_array
					fd_layer3_map[rfd] = AF_INET;
					msg(MSG_DEBUG, "IpfixReceiverSctp: Client connected from %s:%d, FD=%d", inet_ntoa(clientAddress4.sin_addr), ntohs(clientAddress4.sin_port), rfd);
					if (rfd > maxfd){
						maxfd = rfd;
					}
				} else {
					msg(MSG_DEBUG, "IpfixReceiverSctp: Connection from unwanted client %s:%d, FD=%d rejected.", inet_ntoa(clientAddress4.sin_addr), ntohs(clientAddress4.sin_port), rfd);
					close(rfd);
				}
			}else{
				msg(MSG_ERROR ,"accept() in ipfixReceiver failed");
				THROWEXCEPTION("IpfixReceiverSctp: unable to accept new connection");
			}
		}
		if (FD_ISSET(socket6, &readfds)){
			rfd = accept(socket6, (struct sockaddr*)&clientAddress6, &clientAddressLen6);
			
			if (rfd >= 0){
				if (isHostAuthorized(&clientAddress6.sin6_addr, sizeof(clientAddress6.sin6_addr))) {
					FD_SET(rfd, &fd_array); // add new client to fd_array
					fd_layer3_map[rfd] = AF_INET6;
					char address[INET6_ADDRSTRLEN];
					inet_ntop(AF_INET6, &clientAddress6.sin6_addr, address, INET6_ADDRSTRLEN);
					msg(MSG_DEBUG, "IpfixReceiverSctp: Client connected from %s:%d, FD=%d", address, ntohs(clientAddress6.sin6_port), rfd);
					if (rfd > maxfd){
						maxfd = rfd;
					}
				} else {
					char address[INET6_ADDRSTRLEN];
					inet_ntop(AF_INET6, &clientAddress6.sin6_addr, address, INET6_ADDRSTRLEN);
					msg(MSG_DEBUG, "IpfixReceiverSctp: Connection from unwanted client %s:%d, FD=%d rejected.", address, ntohs(clientAddress6.sin6_port), rfd);
					close(rfd);
				}
			}else{
				msg(MSG_ERROR ,"accept() in ipfixReceiver failed");
				THROWEXCEPTION("IpfixReceiverSctp: unable to accept new connection");
			}
		}
		// check all connected sockets for new available data
		for (rfd = firstNoneControl; rfd <= maxfd; ++rfd) {
      			if (FD_ISSET(rfd, &readfds)) {
				boost::shared_array<uint8_t> data(new uint8_t[MAX_MSG_LEN]);
				if (fd_layer3_map[rfd] == AF_INET) 
	      				ret = recvfrom(rfd, data.get(), MAX_MSG_LEN, 0, (struct sockaddr*)&clientAddress4, &clientAddressLen4);
				else
					ret = recvfrom(rfd, data.get(), MAX_MSG_LEN, 0, (struct sockaddr*)&clientAddress6, &clientAddressLen6);
				if (ret == 0) { // this was a shut down (or error)
					FD_CLR(rfd, &fd_array); // delete dead client

					if (fd_layer3_map[rfd] == AF_INET) {
						msg(MSG_DEBUG, "IpfixReceiverSctp: Client %s disconnected", inet_ntoa(clientAddress4.sin_addr));
					} else {
						char address[INET6_ADDRSTRLEN];
						inet_ntop(AF_INET6, &clientAddress6.sin6_addr, address, INET6_ADDRSTRLEN);
						msg(MSG_DEBUG, "IpfixReceiverSctp: Client %s disconnected", address);
					}
				}
				if (ret < 0) { // error
					if (fd_layer3_map[rfd] == AF_INET) {
						msg(MSG_ERROR, "IpfixReceiverSctp: Client error (%s), close connection.", inet_ntoa(clientAddress4.sin_addr));
					} else {
						char address[INET6_ADDRSTRLEN];
						inet_ntop(AF_INET6, &clientAddress6.sin6_addr, address, INET6_ADDRSTRLEN);
						msg(MSG_ERROR, "IpfixReceiverSctp: Client error (%s), close connection.", address);
					}
					fd_layer3_map.erase(rfd);
					close(rfd);
					// we treat an error like a shut down, so overwrite return value to zero
					ret = 0;
				}
				// create sourceId
				boost::shared_ptr<IpfixRecord::SourceID> sourceID(new IpfixRecord::SourceID);
				if (fd_layer3_map[rfd] == AF_INET) {
					memcpy(sourceID->exporterAddress.ip, &clientAddress4.sin_addr.s_addr, 4);
					sourceID->exporterAddress.len = 4;
					sourceID->exporterPort = ntohs(clientAddress4.sin_port);
				} else {
					memcpy(sourceID->exporterAddress.ip, &clientAddress6.sin6_addr, 16);
					sourceID->exporterAddress.len = 16;
					sourceID->exporterPort = ntohs(clientAddress6.sin6_port);
				}
				sourceID->protocol = IPFIX_protocolIdentifier_SCTP;
				sourceID->receiverPort = receiverPort;
				sourceID->fileDescriptor = rfd;
				// send packet to all packet processors
				mutex.lock();
				for (std::list<IpfixPacketProcessor*>::iterator i = packetProcessors.begin(); i != packetProcessors.end(); ++i) { 
					(*i)->processPacket(data, ret, sourceID);
				}
				mutex.unlock();
      			}
      		}
	}
	msg(MSG_DEBUG, "IpfixReceiverSctp: Exiting");
}

#endif /*SUPPORT_SCTP*/
