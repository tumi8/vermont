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

#include "IpfixReceiver.hpp"

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


IpfixReceiver::IpfixReceiver()
	: exitFlag(true),
	  vmodule(NULL),
	  thread(threadWrapper),
	  socket4(-1),
	  socket6(-1)
{
}

IpfixReceiver::IpfixReceiver(int port)
	: exitFlag(true),
	  receiverPort(port),
	  thread(threadWrapper),
	  socket4(-1),
	  socket6(-1)
	  
{
}

/**
 * Frees memory used by an IpfixReceiver.
 */
IpfixReceiver::~IpfixReceiver() 
{
	close(socket4);
	close(socket6);
}


/**
 * Starts processing messages.
 * All sockets prepared by calls to @c createIpfixReceiver() will start
 */
void IpfixReceiver::performStart() 
{
	exitFlag = false;
	thread.run(this);
	
	std::list<IpfixPacketProcessor*>::iterator iter = packetProcessors.begin();
	while (iter != packetProcessors.end()) {
		(*iter)->performStart();
		iter++;
	}
}

/**
 * Stops processing messages.
 * No more messages will be processed until the next startIpfixReceiver() call.
 */
void IpfixReceiver::performShutdown() 
{
	// stop receiver thread
	exitFlag = true;
	thread.join();

	// shutdown packet processors
	std::list<IpfixPacketProcessor*>::iterator iter = packetProcessors.begin();
	while (iter != packetProcessors.end()) {
		(*iter)->performShutdown();
		iter++;
	}
}

/**
 * Assigns a list of packetProcessors to the Receiver. The list has to be managed (creation and
 * destruction) by the calling instance.
 * @param ipfixReceiver handle of receiver, the packetProcessors should be assigned to
 * @param packetProcessor List of PacketProcessors
 * @return 0 on success, non-zero on error
 */
int IpfixReceiver::setPacketProcessors(std::list<IpfixPacketProcessor*> packetProcessors) 
{
	this->packetProcessors = packetProcessors;
	return 0;
}

/**
 * Checks if PacketProcessors where assigned to the IpfixReceiver
 * @return 0 if no PacketProcessors where assigned, > 0 otherwise
 */
bool IpfixReceiver::hasPacketProcessor() 
{
	return !packetProcessors.empty();
}

/**
 * Adds a struct in_addr or in_addr6 to the list of hosts we accept packets from
 * @param ipfixReceiver IpfixReceiver to set the callback function for
 * @param host address to add to the list
 * @return 0 on success, non-zero on error
 */
int IpfixReceiver::addAuthorizedHost(const char* host) 
{
	struct in_addr inaddr;
	struct in6_addr inaddr6;

	enum receiver_address_type addrType = getAddressType(host);
	if (addrType == IPv4_ADDRESS) {
		if (inet_aton(host, &inaddr) == 0) {
			// This may not happen. We checked previously 
			// that this IP address is an IPv4 address
			THROWEXCEPTION("Could not convert %s to in_addr. This is a bug. Please report it!", host);
		}
		authHosts4.push_back(inaddr);
	} else if (addrType == IPv6_ADDRESS) {
		if (inet_pton(AF_INET6, host, &inaddr6) == 0) {
			// This may not happen. We checked previously 
			// that this IP address is an IPv6 address
			THROWEXCEPTION("Could not convert %s to in_addr6. This is a bug. Please report it!", host);
		}
		authHosts6.push_back(inaddr6);
	} else {
		msg(MSG_ERROR, "Cannot add host to authorized host list. Invalid address type: %s", host);
		return -1;
	}
	return 0;
}

/**
 * Checks if a given host is a member of the list of authorized hosts
 * @param ipfixReceiver handle to an IpfixReceiver
 * @param inaddr Ipv4 Address of the host to check
 * @param addrlen Length of inaddr
 * @return 0 if host is NOT in list, non-zero otherwise
 */
int IpfixReceiver::isHostAuthorized(struct in_addr* inaddr, int addrlen) 
{
	/* if we have a list of authorized hosts, discard message if sender is not in this list */
	if (!authHosts4.empty()) {
		for (unsigned i=0; i < authHosts4.size(); ++i) {
			if (memcmp(inaddr, &authHosts4[i], addrlen) == 0)
				return 1;
		}
		/* isn't in list */
		return 0;
	}
	return 1;
}

/**
 * Checks if a given host is a member of the list of authorized hosts
 * @param ipfixReceiver handle to an IpfixReceiver
 * @param inaddr Address of the host to check
 * @param addrlen Length of inaddr
 * @return 0 if host is NOT in list, non-zero otherwise
 */
int IpfixReceiver::isHostAuthorized(struct in6_addr* inaddr, int addrlen) 
{
	/* if we have a list of authorized hosts, discard message if sender is not in this list */
	if (!authHosts6.empty()) {
		for (unsigned i=0; i < authHosts6.size(); ++i) {
			if (memcmp(inaddr, &authHosts6[i], addrlen) == 0)
				return 1;
		}
		/* isn't in list */
		return 0;
	}
	return 1;
}


/**
 * Thread function responsible for receiving packets from the network
 * @param ipfixReceiver_ pointer to an IpfixReceiver instance
 * @return NULL
 */
void* IpfixReceiver::threadWrapper(void* ipfixReceiver_) {
   IpfixReceiver* ipfixReceiver = (IpfixReceiver*)ipfixReceiver_;

   ipfixReceiver->vmodule->registerCurrentThread();
   
   ipfixReceiver->run();

   ipfixReceiver->vmodule->unregisterCurrentThread();

   return NULL;
}


void IpfixReceiver::setVModule(Module* m)
{
	vmodule = m;
}


enum IpfixReceiver::receiver_address_type IpfixReceiver::getAddressType(const std::string& addr)
{
	struct addrinfo hint, *res = NULL;
	enum receiver_address_type ret = INVALID;
	int addRet;
	
	memset(&hint, 0, sizeof hint);

	hint.ai_family = PF_UNSPEC;
	hint.ai_flags |= AI_NUMERICHOST;
	
	addRet = getaddrinfo(addr.c_str(), NULL, &hint, &res);
	if (addRet) {
		msg(MSG_FATAL, "Invalid address: %s", gai_strerror(addRet));
	        ret = INVALID;
	}
	if(res->ai_family == AF_INET) {
		msg(MSG_DEBUG, "Collector IP \"%s\" is an IPv4 address", addr.c_str());
		ret = IPv4_ADDRESS;
	} else if (res->ai_family == AF_INET6) {
		msg(MSG_DEBUG, "Collector IP \"%s\": is an IPv6 address", addr.c_str());
		ret = IPv6_ADDRESS;
	} else {
		msg(MSG_DEBUG, "Collector IP \"%s\" is an is unsupported address format: %d\n", addr.c_str(), res->ai_family);
		ret = INVALID;
	}
   	freeaddrinfo(res);

	return ret;
}

void IpfixReceiver::createIPv4Socket(const std::string& ipAddr, int type, int subtype, uint16_t port)
{
	struct sockaddr_in serverAddress;
	socket4 = socket(AF_INET, type, subtype);
	if(socket4 < 0) {
		THROWEXCEPTION("Cannot create IpfixReceiverUdpIpV4, socket creation failed: %s", strerror(errno));
	}
	if (ipAddr == "") {
		serverAddress.sin_addr.s_addr = INADDR_ANY;
	} else {
		if (inet_pton(AF_INET, ipAddr.c_str(), &serverAddress) <= 0) {
			THROWEXCEPTION("Could not convert Collector \"%s\" to IPv4 address: %s", ipAddr.c_str(), strerror(errno));
		}
	}

	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(port);
	if(bind(socket4, (struct sockaddr*)&serverAddress, sizeof(struct sockaddr_in)) < 0) {
		THROWEXCEPTION("Cannot create IpfixReceiverUdpIpV4 %s:%d",ipAddr.c_str(), port );
	}
}

void IpfixReceiver::createIPv6Socket(const std::string& ipAddr, int type, int subtype, uint16_t port)
{
	struct sockaddr_in6 serverAddress;
		
	socket6 = socket(AF_INET6, type, subtype);
	if(socket6 < 0) {
		THROWEXCEPTION("Cannot create IpfixReceiverUdpIpV4, socket creation failed: %s", strerror(errno));
	}
	if (ipAddr == "") {
		serverAddress.sin6_addr = in6addr_any;
	} else {
		if (inet_pton(AF_INET6, ipAddr.c_str(), &serverAddress) <= 0) {
			THROWEXCEPTION("Could not convert Collector \"%s\" to IPv6 address: %s", ipAddr.c_str(), strerror(errno));
		}
	}
	serverAddress.sin6_family = AF_INET6;
	serverAddress.sin6_port = htons(port);
	if(bind(socket6, (struct sockaddr*)&serverAddress, sizeof(struct sockaddr_in6)) < 0) {
		THROWEXCEPTION("Could not bind socket: %s", strerror(errno));
	}
}
 
