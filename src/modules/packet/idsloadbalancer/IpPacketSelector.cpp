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
#include "common/Misc.h"
#include <sstream>

IpPacketSelector::IpPacketSelector()
	: BasePacketSelector("IpPacketSelector")
{
}

IpPacketSelector::~IpPacketSelector()
{
}


void IpPacketSelector::setSubnets(list<uint32_t> subnets, list<uint32_t> masks)
{
	this->subnets = subnets;
	this->masks = masks;

	ostringstream oss;
	oss << "IpPacketSelector: subnet and mask filter set to ";
	list<uint32_t>::iterator siter = subnets.begin();
	list<uint32_t>::iterator miter = masks.begin();
	while (siter!=subnets.end()) {
		oss << IPToString(*siter) << "/" << IPToString(*miter);
		siter++;
		miter++;
	}
	msg(MSG_INFO, oss.str().c_str());
}


int IpPacketSelector::decide(Packet *p)
{
	if (!p->customData) {
		THROWEXCEPTION("IpPacketSelector: no custom data in packet found, but required");
	}
	PacketHostInfo** phi = reinterpret_cast<PacketHostInfo**>(p->customData);
	HostHashtable* hosts = phi[0]->selectorData;

	uint32_t src = 	*((uint32_t *)(p->netHeader + SRC_ADDRESS_OFFSET));
	uint32_t dst = 	*((uint32_t *)(p->netHeader + DST_ADDRESS_OFFSET));
	
	list<uint32_t>::iterator siter = subnets.begin();
	list<uint32_t>::iterator miter = masks.begin();
	while (siter!=subnets.end()) {
		if ((src&*miter)==*siter) {
			HostHashtable::iterator res = hosts->find(src);
			if (res != hosts->end()) {
				int qid = res->second.queueid;
				PacketHostInfo* ph = phi[qid];
				if (ph->dropModulo>1 && (((ntohl(src)^ph->salt)%ph->dropModulo)!=0)) {
					ph->controlDropped += p->data_length;
					break;
				}
				return qid;
			}
		}
		if ((dst&*miter)==*siter) {
			HostHashtable::iterator res = hosts->find(dst);
			if (res != hosts->end()) {
				int qid = res->second.queueid;
				PacketHostInfo* ph = phi[qid];
				if (ph->dropModulo>1 && (((ntohl(dst)^ph->salt)%ph->dropModulo)!=0)) {
					ph->controlDropped += p->data_length;
					break;
				}
				return qid;
			}
		}
		siter++;
		miter++;
	}
	return -1;
}

