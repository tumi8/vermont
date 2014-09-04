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

#ifndef _IPFIX_RECEIVER_UDPIPV4_H_
#define _IPFIX_RECEIVER_UDPIPV4_H_

#include <pthread.h>
#include <stdint.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <list>

#include "IpfixReceiver.hpp"
#include "IpfixPacketProcessor.hpp"

class IpfixReceiverUdpIpV4 : public IpfixReceiver, Sensor {
	public:
		IpfixReceiverUdpIpV4(int port, std::string ipAddr = "", const uint32_t buffer = 0);
		virtual ~IpfixReceiverUdpIpV4();

		virtual void run();
		virtual std::string getStatisticsXML(double interval);
		
	private:
		int listen_socket;
		uint32_t statReceivedPackets;  /**< number of received packets */ 
};

#endif
