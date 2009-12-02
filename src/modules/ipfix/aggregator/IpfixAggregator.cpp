/*
 * IPFIX Concentrator Module Library
 * Copyright (C) 2004 Christoph Sommer <http://www.deltadevelopment.de/users/christoph/ipfix/>
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

#include <stdexcept>
#include <netinet/in.h>
#include <unistd.h>
#include "IpfixAggregator.hpp"
#include "modules/ipfix/IpfixParser.hpp"
#include "modules/ipfix/IpfixSender.hpp"
#include "FlowHashtable.h"

#include "common/msg.h"




/**
 * constructs a new instance
 * @param pollinterval sets the interval of polling the hashtable for expired flows
 */
IpfixAggregator::IpfixAggregator(uint32_t pollinterval)
	: BaseAggregator(pollinterval)
{	
}


IpfixAggregator::~IpfixAggregator()
{
	
}


/**
 * Injects new DataRecords into the Aggregator.
 * @param sourceID ignored
 * @param ti structure describing @c data
 * @param length length (in bytes) of @c data
 * @param data raw data block containing the Record
 */
void IpfixAggregator::onDataRecord(IpfixDataRecord* record)
{
	DPRINTF("Got a Data Record\n");

#if defined(DEBUG)
	if(!rules) {
		THROWEXCEPTION("Aggregator not started");
	}
#endif
	
	TemplateInfo* ti = record->templateInfo.get();

	
	for (size_t i = 0; i < rules->count; i++) {
		if (rules->rule[i]->templateDataMatches(ti, record->data)) {
			DPRINTF("rule %d matches", i);

			static_cast<FlowHashtable*>(rules->rule[i]->hashtable)->aggregateTemplateData(ti, record->data);
		}
	}

	record->removeReference();
}


/**
 * Injects new DataRecords into the Aggregator.
 * @param sourceID ignored
 * @param ti structure describing @c data
 * @param length length (in bytes) of @c data
 * @param data raw data block containing the Record
 */
void IpfixAggregator::onDataDataRecord(IpfixDataDataRecord* record)
{
	DPRINTF("onDataDataRecord: Got a DataData Record\n");

#if defined(DEBUG)
	if(!rules) {
		THROWEXCEPTION("Aggregator not started");
	}
#endif

	TemplateInfo* ti = record->dataTemplateInfo.get();

	mutex.lock();
	for (size_t i = 0; i < rules->count; i++) {
		if (rules->rule[i]->dataTemplateDataMatches(ti, record->data)) {
			DPRINTF("rule %d matches\n", i);
			static_cast<FlowHashtable*>(rules->rule[i]->hashtable)->aggregateDataTemplateData(ti, record->data);
		}
	}
	mutex.unlock();

	record->removeReference();
}


/**
 * creates hashtable for this aggregator
 */
BaseHashtable* IpfixAggregator::createHashtable(Rule* rule, uint16_t minBufferTime, 
		uint16_t maxBufferTime, uint8_t hashbits)
{
	return new FlowHashtable(this, rule, minBufferTime, maxBufferTime, hashbits);
}

