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

#include "IpPacketSelector.hpp"

IpPacketSelector::IpPacketSelector()
	: BasePacketSelector("IpPacketSelector")
{
}
IpPacketSelector::~IpPacketSelector()
{
}
void IpPacketSelector::initializeConfig(std::map<uint32_t, int>& src, std::map<uint32_t, int>& dst){
	srcips = src;
	dstips = dst;
}
	
void IpPacketSelector::addDestinationIp(uint32_t dst, int queueno)
{
	dstips[dst] = queueno;
}

void IpPacketSelector::addSourceIp(uint32_t src, int queueno)
{
	srcips[src] = queueno;
}

int IpPacketSelector::decide(Packet *p)
{
	//msg(MSG_INFO, "decider: %d, %d", srcips.size(), dstips.size());
	uint32_t src = 	*((uint32_t *)(p->netHeader + SRC_ADDRESS_OFFSET));
	uint32_t dst = 	*((uint32_t *)(p->netHeader + DST_ADDRESS_OFFSET));
	
	if(srcips.find(src) != srcips.end())
		return srcips[src];
	if(dstips.find(dst) != dstips.end())
		return dstips[dst];
	return -1;

}
