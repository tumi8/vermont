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
#ifndef _IPFIX_RECEIVER_SCTPIPV4_H_
#define _IPFIX_RECEIVER_SCTPIPV4_H_

#include <pthread.h>
#include <stdint.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <list>

#include "IpfixReceiver.hpp"
#include "IpfixPacketProcessor.hpp"

// Quote from man page: "maximum length to which the queue of pending connections
// for sockfd may grow. If a connection request arrives when the queue is
// full, the client may receive an error with an indication of ECONNREFUSED
// or, if the underlying protocol supports retransmission, the request may
// be ignored so that a later reattempt at connection succeeds."
#define SCTP_MAX_BACKLOG 5


class IpfixReceiverSctpIpV4 : public IpfixReceiver, Sensor {
#ifdef SUPPORT_SCTP
	public:
		IpfixReceiverSctpIpV4(int port, std::string ipAddr = "", uint32_t buffer = 0);
		virtual ~IpfixReceiverSctpIpV4();

		virtual void run();
		std::string getStatisticsXML(double interval);

	private:
		int listen_socket;
		uint32_t statReceivedPackets;  /**< number of received packets */ 
#else
	public:
		IpfixReceiverSctpIpV4(int port, std::string ipAddr, uint32_t buffer = 0) {
			THROWEXCEPTION("SCTP not supported!");
		}
		
		virtual ~IpfixReceiverSctpIpV4() {}

		virtual void run() {}

#endif /*SUPPORT_SCTP*/
};

#endif

