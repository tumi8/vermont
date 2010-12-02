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
	: BasePacketSelector("IpPacketSelector"),
	  changelists(false)
{
}
IpPacketSelector::~IpPacketSelector()
{
}



void IpPacketSelector::initializeConfig(boost::unordered_map<uint32_t, int>& src, boost::unordered_map<uint32_t, int>& dst)
{
	newsrcips = src;
	newdstips = dst;
	changelists = true;
}

void IpPacketSelector::setSubnets(list<uint32_t> subnets, list<uint32_t> masks)
{
	this->subnets = subnets;
	this->masks = masks;
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
	if (changelists) {
		changelists = false;
		srcips = newsrcips;
		dstips = newdstips;
	}
	//msg(MSG_INFO, "decider: %d, %d", srcips.size(), dstips.size());
	uint32_t src = 	*((uint32_t *)(p->netHeader + SRC_ADDRESS_OFFSET));
	uint32_t dst = 	*((uint32_t *)(p->netHeader + DST_ADDRESS_OFFSET));
	
	list<uint32_t>::iterator siter = subnets.begin();
	list<uint32_t>::iterator miter = masks.begin();
	while (siter!=subnets.end()) {
		if ((src&*miter)==*siter) {
			boost::unordered_map<uint32_t, int>::iterator res = srcips.find(src);
			if (res != srcips.end()) {
				return res->second;
			}
		}
		if ((dst&*miter)==*siter) {
			boost::unordered_map<uint32_t, int>::iterator res = srcips.find(dst);
			if (res != dstips.end()) {
				return res->second;
			}
		}
		siter++;
		miter++;
	}
	return -1;

}
