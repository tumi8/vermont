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


/**
 * Creates a new Aggregator.
 * @param ruleFile filename of file containing a set of rules
 * @param minBufferTime timeout when passive flows are exported
 * @param maxBufferTime timeout when active flows are exported
 */
PacketAggregator::PacketAggregator(char* ruleFile, uint16_t minBufferTime, uint16_t maxBufferTime)
{
	Rules* rules = new Rules(ruleFile);

	if (!rules) {
		THROWEXCEPTION("could not parse rules file %s", ruleFile);
	}
	buildAggregator(rules, minBufferTime, maxBufferTime);

	setSinkOwner("PacketAggregator");
}

/**
 * Creates a new Aggregator. Do not forget to set the callback functions, then call @c startAggreagtor().
 * @param rules Rules for aggregator to work with
 * @param minBufferTime timeout when passive flows are exported
 * @param maxBufferTime timeout when active flows are exported
 */
PacketAggregator::PacketAggregator(Rules* rules, uint16_t minBufferTime, uint16_t maxBufferTime)
{
	buildAggregator(rules, minBufferTime, maxBufferTime);

	setSinkOwner("PacketAggregator");
}

PacketAggregator::~PacketAggregator()
{
	int i;
	for (i = 0; i < rules->count; i++) {
		delete ((Hashtable*)rules->rule[i]->hashtable);
	}
	delete rules;

	pthread_mutex_unlock(&mutex);
	pthread_mutex_destroy(&mutex);
}

/**
 * Builds a new aggregator from the given rules
 */
void PacketAggregator::buildAggregator(Rules* rules, uint16_t minBufferTime, uint16_t maxBufferTime)
{
	int i;

	this->rules = rules;

	for (i = 0; i < rules->count; i++) {
		rules->rule[i]->initialize();
		rules->rule[i]->hashtable = new Hashtable(rules->rule[i], minBufferTime, maxBufferTime);
	}

	if (pthread_mutex_init(&mutex, NULL) != 0) {
		msg(MSG_FATAL, "Could not init mutex");
	}
		
	if (pthread_mutex_lock(&mutex) != 0) {
		msg(MSG_FATAL, "Could not lock mutex");
	}

	msg(MSG_INFO, "Done. Parsed %d rules; minBufferTime %d, maxBufferTime %d", rules->count, minBufferTime, maxBufferTime);
}

void PacketAggregator::start()
{
	pthread_mutex_unlock(&mutex);
}

void PacketAggregator::stop()
{
	pthread_mutex_lock(&mutex);
}
	
void PacketAggregator::receive(Packet* e)
{
	
}
