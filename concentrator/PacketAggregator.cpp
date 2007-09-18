/*
 * PacketAggregator
 * Copyright (C) 2007 Tobias Limmer <tobias.limmer@informatik.uni-erlangen.de>
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

#include "PacketAggregator.h"

#include "PacketHashtable.h"


/**
 * constructs a new instance
 * @param pollinterval sets the interval of polling the hashtable for expired flows
 */
PacketAggregator::PacketAggregator(uint32_t pollinterval)
	: BaseAggregator(pollinterval)
{	
}


PacketAggregator::~PacketAggregator()
{
}

	
/**
 * aggregates given packet
 */
void PacketAggregator::receive(Packet* e)
{
	int i;

#if defined(DEBUG)
	if(!rules) {
		THROWEXCEPTION("Aggregator not started");
	}
#endif

	mutex.lock();
	for (i = 0; i < rules->count; i++) {
		if (rules->rule[i]->ExptemplateDataMatches(e)) {
			DPRINTF("rule %d matches\n", i);
			static_cast<PacketHashtable*>(rules->rule[i]->hashtable)->aggregatePacket(e);
		}
	}
	mutex.unlock();
}


/**
 * creates hashtable for this aggregator
 */
BaseHashtable* PacketAggregator::createHashtable(Rule* rule, uint16_t minBufferTime, 
		uint16_t maxBufferTime)
{
	return new PacketHashtable(this, rule, minBufferTime, maxBufferTime);
}
