/*
 * Vermont IDS Load Balancer
 * Copyright (C) 2010 Vermont Project
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

#ifndef BASE_PACKET_SELECTOR_HPP
#define BASE_PACKET_SELECTOR_HPP

#include "modules/packet/Packet.h"
#include "IDSLoadStatistics.hpp"

#define SRC_ADDRESS_OFFSET 12
#define DST_ADDRESS_OFFSET 16
#define TCP_UDP_SRC_PORT_OFFSET 0
#define TCP_UDP_DST_PORT_OFFSET 4

class Packet;

class BasePacketSelector {
	public:
		BasePacketSelector(string name);
		~BasePacketSelector();

		virtual void setQueueCount(uint32_t n);

		/**
		 * @returns ID of queue where packet should be inserted. If it is to be dropped, -1 is returned
		 */
		virtual int decide(Packet *p) = 0;
		virtual void updateData(list<IDSLoadStatistics>& lstats) {}
		virtual void setUpdateInterval(uint32_t ms) {}
		virtual string getName();
		virtual void start() {}
		virtual void stop() {}

	protected:
		uint32_t numberOfQueues;
		string name;

};

#endif
