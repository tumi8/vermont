/*
 * IPFIX Concentrator Module Library - TCP Receiver
 * Copyright (C) 2011 Lothar Braun <braun@net.in.tum.de>
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
#ifndef _IPFIX_RECEIVER_TCPIPV4_H_
#define _IPFIX_RECEIVER_TCPIPV4_H_

#include <pthread.h>
#include <stdint.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <list>

#include "IpfixReceiver.hpp"
#include "IpfixPacketProcessor.hpp"

#define TCP_MAX_BACKLOG 5

class IpfixReceiverTcpIpV4 : public IpfixReceiver, Sensor {
	public:
		IpfixReceiverTcpIpV4(int port, std::string ipAddr = "", const uint32_t buffer = 0);
		virtual ~IpfixReceiverTcpIpV4();

		virtual void run();
		virtual std::string getStatisticsXML(double interval);
	private:
		int listen_socket;
		uint32_t statReceivedMessages;  /**< number of received messages */
};

#endif

