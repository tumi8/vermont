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

#ifndef _IPFIX_RECEIVER_H_
#define _IPFIX_RECEIVER_H_

#include "core/Module.h"
#include "IpfixPacketProcessor.hpp"

#include <pthread.h>
#include <stdint.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <list>



/**
 * Control structure for receiving process.
 */
class IpfixReceiver
{
	public:
		IpfixReceiver();
		IpfixReceiver(int port);
		virtual ~IpfixReceiver();

		void performStart();
		void performShutdown();

		int addAuthorizedHost(const char* host);
		int isHostAuthorized(struct in_addr* inaddr, int addrlen);
		int setPacketProcessors(std::list<IpfixPacketProcessor*> packetProcessors);
		bool hasPacketProcessor();
		void setVModule(Module* m);
		
		virtual void run() = 0;

	protected:
		std::list<IpfixPacketProcessor*> packetProcessors; /**< Authorized incoming packets are forwarded to the packetProcessors. The list of packetProcessor must be created, managed and destroyed by an superior instance. The IpfixReceiver will only work with the given list */
		bool exitFlag;
	
		Mutex mutex;
		uint16_t receiverPort;
		Module* vmodule;
		void setBufferSize(const int sockfd, const uint32_t buffer);

	private:
		std::vector<in_addr> authHosts; /**< List of authorized hosts. Only packets from hosts in this list, will be forwarded to the PacketProcessors */
		Thread thread;
	
	
		static void* threadWrapper(void* instance);
};

#endif
