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

#ifndef HASH_PACKET_SELECTOR_HPP
#define HASH_PACKET_SELECTOR_HPP

#include "BasePacketSelector.hpp"
#include "../Packet.h"
#include <map>
#include <stdint.h>

class Packet;

class HashPacketSelector : public BasePacketSelector 
{
	public:
		~HashPacketSelector();
		HashPacketSelector();
		virtual int decide(Packet *p);
		void addDestinationIp(uint32_t dst, int queueno);
		void addSourceIp(uint32_t src, int queueno);
		
	private:
		std::map<uint32_t, int> dstips;
		std::map<uint32_t, int> srcips;

};

#endif
