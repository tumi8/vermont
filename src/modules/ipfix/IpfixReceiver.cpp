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
	  thread(threadWrapper, "IpfixReceiver")
{
}

IpfixReceiver::IpfixReceiver(int port)
	: exitFlag(true),
	  receiverPort(port),
	  thread(threadWrapper, "IpfixReceiver")
	  
{
}

/**
 * Frees memory used by an IpfixReceiver.
 */
IpfixReceiver::~IpfixReceiver() 
{

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
 * Adds a struct in_addr to the list of hosts we accept packets from
 * @param ipfixReceiver IpfixReceiver to set the callback function for
 * @param host address to add to the list
 * @return 0 on success, non-zero on error
 */
int IpfixReceiver::addAuthorizedHost(const char* host) 
{
	struct in_addr inaddr;

	if (inet_aton(host, &inaddr) == 0) {
		msg(MSG_ERROR, "Invalid host address: %s", host);
		return -1;
	}

	authHosts.push_back(inaddr);
	return 0;
}

/**
 * Checks if a given host is a member of the list of authorized hosts
 * @param ipfixReceiver handle to an IpfixReceiver
 * @param inaddr Address of the host to check
 * @param addrlen Length of inaddr
 * @return 0 if host is NOT in list, non-zero otherwise
 */
int IpfixReceiver::isHostAuthorized(struct in_addr* inaddr, int addrlen) 
{
	/* if we have a list of authorized hosts, discard message if sender is not in this list */
	if (!authHosts.empty()) {
		for (unsigned i=0; i < authHosts.size(); ++i) {
			if (memcmp(inaddr, &authHosts[i], addrlen) == 0)
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


/**
 * Set receiver buffer size in kernel
 * @param buffer buffer size in bytes
 * @return NULL
 */
void IpfixReceiver::setBufferSize(const int sockfd, const uint32_t buffer)
{
    if (buffer != 0) {
        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &buffer, sizeof(uint32_t)) == -1) {
            msg(MSG_ERROR, "Error setting socket buffer size: %s", strerror(errno));
        }
		uint32_t temp;
		socklen_t len = sizeof(temp);
		getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &temp, &len);
		msg(MSG_INFO, "Socket buffer size set to %lu bytes", temp);
    }
}
