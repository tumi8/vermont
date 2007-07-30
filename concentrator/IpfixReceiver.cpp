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
#include "IpfixParser.hpp"
#include "IpfixReceiverUdpIpV4.hpp"
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


IpfixReceiver::IpfixReceiver() {
}

/**
 * Frees memory used by an IpfixReceiver.
 * Attention: Memory of the assigned PacketProcessors is NOT freed.
 * This has to be done by the calling instance itself.
 * @param ipfixReceiver Handle returned by @c createIpfixReceiver()
 */
IpfixReceiver::~IpfixReceiver() {
	/* general cleanup */
	
	//FIXME: cleanup listener thread

	if (pthread_mutex_unlock(&mutex) != 0) {
		msg(MSG_FATAL, "Could not unlock mutex");
	}

	pthread_mutex_destroy(&mutex);
}


/**
 * Starts processing messages.
 * All sockets prepared by calls to @c createIpfixReceiver() will start
 * receiving messages until @c stopIpfixReceiver() is called.
 * @return 0 on success, non-zero on error
 */
int IpfixReceiver::start() {
	exit = 0;
	
	if (pthread_mutex_init(&mutex, NULL) != 0) {
		msg(MSG_FATAL, "IpfixReceiver::start: Could not init mutex");
		goto out1;
	}
	
	if (pthread_mutex_lock(&mutex) != 0) {
		msg(MSG_FATAL, "IpfixReceiver::start: Could not lock mutex");
		goto out1;
	}
	
	if(pthread_create(&(thread), 0, listenerThread, this) != 0) {
		msg(MSG_FATAL, "IpfixReceiver::start: Could not create listener thread");
		goto out1;
	}

	if (pthread_mutex_unlock(&mutex) != 0) {
		msg(MSG_FATAL, "IpfixReceiver::start: Could not unlock mutex");
		return -1;
	}
	
	return 0;
out1:
	THROWEXCEPTION("IpfixReceiver::startThread: thread creation failed");
	return 1;
}

/**
 * Stops processing messages.
 * No more messages will be processed until the next startIpfixReceiver() call.
 * @return 0 on success, non-zero on error
 */
int IpfixReceiver::stop() {
	if (pthread_mutex_lock(&mutex) != 0) {
		msg(MSG_FATAL, "IpfixReceiver::stop(): Could not lock mutex");
		return -1;
	}

	void* result = NULL;
        if (pthread_join(thread, &result)) {
		msg(MSG_FATAL, "IpfixReceiver::stop(): Could not join thread");
                return -1;
        }
            
	return 0;
}

/**
 * Assigns a list of packetProcessors to the Receiver. The list has to be managed (creation and
 * destruction) by the calling instance.
 * @param ipfixReceiver handle of receiver, the packetProcessors should be assigned to
 * @param packetProcessor List of PacketProcessors
 * @return 0 on success, non-zero on error
 */
int IpfixReceiver::setPacketProcessors(std::list<IpfixPacketProcessor*> packetProcessors) {
	this->packetProcessors = packetProcessors;
	return 0;
}

/**
 * Checks if PacketProcessors where assigned to the IpfixReceiver
 * @return 0 if no PacketProcessors where assigned, > 0 otherwise
 */
bool IpfixReceiver::hasPacketProcessor() {
	return !packetProcessors.empty();
}

/**
 * Adds a struct in_addr to the list of hosts we accept packets from
 * @param ipfixReceiver IpfixReceiver to set the callback function for
 * @param host address to add to the list
 * @return 0 on success, non-zero on error
 */
int IpfixReceiver::addAuthorizedHost(const char* host) {
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
int IpfixReceiver::isHostAuthorized(struct in_addr* inaddr, int addrlen) {
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
void* IpfixReceiver::listenerThread(void* ipfixReceiver_) {
   IpfixReceiver* ipfixReceiver = (IpfixReceiver*)ipfixReceiver_;

   ipfixReceiver->run();

   return NULL;
}

/**
 * Called by the logger timer thread. Dumps info using msg_stat
 */
void IpfixReceiver::stats()
{
	msg_stat("Concentrator: IpfixReceiver: %6d records received", receivedRecords);
	receivedRecords = 0;
}

