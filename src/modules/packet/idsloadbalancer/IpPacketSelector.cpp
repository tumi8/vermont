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

#include "PriorityPacketSelector.hpp"

IpPacketSelector::IpPacketSelector()
	: BasePacketSelector("IpPacketSelector"),
	  hosts(0),
	  newHosts(0),
	  changelists(false)
{
}

IpPacketSelector::~IpPacketSelector()
{
}

void IpPacketSelector::setHosts(HostHashtable* hosts)
{
	if (numberOfQueues==0) {
		THROWEXCEPTION("IpPacketSelector: internal error, queue count has not been set to a value >0 when setHosts() was called");
	}

	// busy wait until lists have been accepted by own thread
	while (changelists) {}
	newHosts = hosts;
	changelists = true;
}

void IpPacketSelector::setSubnets(list<uint32_t> subnets, list<uint32_t> masks)
{
	this->subnets = subnets;
	this->masks = masks;
}

void IpPacketSelector::increaseDropModulo(uint32_t queueid)
{
	dropModulo[queueid] <<= 1;
	msg(MSG_INFO, "increasing drop modulo for queue id %u to %u", queueid, dropModulo[queueid]);
}

int IpPacketSelector::decide(Packet *p)
{
	if (changelists) {
		dropModulo.resize(numberOfQueues, 1);
		for (vector<uint32_t>::iterator iter = dropModulo.begin(); iter!=dropModulo.end(); iter++)
			*iter = 1;
		salt += 1;
		hosts = newHosts;
		changelists = false;
	}
	if (p->customData) {
		PacketHostInfo* phi = reinterpret_cast<PacketHostInfo*>(p->customData);
		hosts = phi->selectorData;
	} else if (hosts==0) {
		return -1;
	}

	uint32_t src = 	*((uint32_t *)(p->netHeader + SRC_ADDRESS_OFFSET));
	uint32_t dst = 	*((uint32_t *)(p->netHeader + DST_ADDRESS_OFFSET));
	
	list<uint32_t>::iterator siter = subnets.begin();
	list<uint32_t>::iterator miter = masks.begin();
	while (siter!=subnets.end()) {
		if ((src&*miter)==*siter) {
			HostHashtable::iterator res = hosts->find(src);
			if (res != hosts->end()) {
				int qid = res->second.queueid;
				if (dropModulo[qid]>1 && (src^salt)%dropModulo[qid]!=0) break;
				return qid;
			}
		}
		if ((dst&*miter)==*siter) {
			HostHashtable::iterator res = hosts->find(dst);
			if (res != hosts->end()) {
				int qid = res->second.queueid;
				if (dropModulo[qid]>1 && (dst^salt)%dropModulo[qid]!=0) break;
				return qid;
			}
		}
		siter++;
		miter++;
	}
	return -1;
}

bool IpPacketSelector::wasIpDropped(uint32_t queueid, uint32_t ip)
{
	if (queueid>=dropModulo.size()) return false;
	return (dropModulo[queueid]>1) && ((ip^salt)%dropModulo[queueid]!=0);
}
