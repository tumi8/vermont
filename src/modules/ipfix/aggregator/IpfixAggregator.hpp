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

#ifndef AGGREGATOR_H
#define AGGREGATOR_H

#include "modules/ipfix/IpfixRecord.hpp"
#include "Rules.hpp"
#include "BaseAggregator.h"
#include "modules/ipfix/IpfixRecordDestination.h"
#include "core/Module.h"


/**
 * Represents an Aggregator.
 *
 * Uses Rules and Hashtable to implement an IPFIX Aggregator.
 */
class IpfixAggregator 
		: public BaseAggregator, public IpfixRecordDestination
{
public:
	IpfixAggregator(uint32_t pollinterval);
	virtual ~IpfixAggregator();

	virtual void onDataRecord(IpfixDataRecord* record);

protected:
	BaseHashtable* createHashtable(Rule* rule, uint16_t minBufferTime, 
			uint16_t maxBufferTime, uint8_t hashbits);
};

#endif
