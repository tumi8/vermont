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

#include <sstream>

/**
 * constructs a new instance
 * @param pollinterval sets the interval of polling the hashtable for expired flows in ms
 */
PacketAggregator::PacketAggregator(uint32_t pollinterval)
	: BaseAggregator(pollinterval),
	  statPacketsReceived(0),
	  statIgnoredPackets(0)
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
#if defined(DEBUG)
	if(!rules) {
		THROWEXCEPTION("Aggregator not started");
	}
#endif

	statPacketsReceived++;

	for (size_t i = 0; i < rules->count; i++) {
		if (rules->rule[i]->ExptemplateDataMatches(e)) {
			DPRINTF("rule %d matches\n", i);
			static_cast<PacketHashtable*>(rules->rule[i]->hashtable)->aggregatePacket(e);
		} else {
			statIgnoredPackets++;
		}
	}
	e->removeReference();
}


/**
 * creates hashtable for this aggregator
 */
BaseHashtable* PacketAggregator::createHashtable(Rule* rule, uint16_t minBufferTime,
		uint16_t maxBufferTime, uint8_t hashbits)
{
	return new PacketHashtable(this, rule, minBufferTime, maxBufferTime, hashbits);
}


string PacketAggregator::getStatisticsXML(double interval)
{
	ostringstream oss;
	oss << "<totalReceivedPackets>" << statPacketsReceived << "</totalReceivedPackets>";
	oss << "<ignoredPackets>" << statIgnoredPackets << "</ignoredPackets>";
	oss << BaseAggregator::getStatisticsXML(interval);

	return oss.str();
}
