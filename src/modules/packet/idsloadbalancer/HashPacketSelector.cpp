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
#include "HashPacketSelector.hpp"

HashPacketSelector::HashPacketSelector() 
{
}
HashPacketSelector::~HashPacketSelector()
{
}
	
int HashPacketSelector::decide(Packet *p)
{
	if (p->ipProtocolType == Packet::TCP || p->ipProtocolType == Packet::UDP){
		return ((
			*((uint32_t *) p->netHeader + SRC_ADDRESS_OFFSET) +
			*((uint32_t *) p->netHeader + DST_ADDRESS_OFFSET) +
			*((uint32_t *) p->transportHeader + TCP_UDP_SRC_PORT_OFFSET) +
			*((uint32_t *) p->transportHeader + TCP_UDP_DST_PORT_OFFSET) 
			) % numberOfQueues);
	}
	return ((*((uint32_t *) p->netHeader + SRC_ADDRESS_OFFSET) + *((uint32_t *) p->netHeader + DST_ADDRESS_OFFSET)) % numberOfQueues);
}

