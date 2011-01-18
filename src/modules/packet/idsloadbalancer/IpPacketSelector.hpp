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

#ifndef IP_PACKET_SELECTOR_HPP
#define IP_PACKET_SELECTOR_HPP

#include "BasePacketSelector.hpp"
#include <stdint.h>
#include <arpa/inet.h>
#include <boost/unordered_map.hpp>
#include <list>
#include <vector>


class Packet;

struct SelectorHostData
{
	SelectorHostData() {}
	SelectorHostData(int queueid)
		: queueid(queueid),
		  hostid(0)
	{}

	int queueid;
	uint32_t hostid; /**< used for fast dropping */
};

typedef boost::unordered_map<uint32_t, SelectorHostData> HostHashtable;


class IpPacketSelector : public BasePacketSelector 
{
	public:
		~IpPacketSelector();
		IpPacketSelector();
		void setSubnets(list<uint32_t> subnets, list<uint32_t> masks);
		void setHosts(HostHashtable* hosts);
		virtual int decide(Packet *p);
		
	private:
		list<uint32_t> subnets;
		list<uint32_t> masks;
}
;

#endif
