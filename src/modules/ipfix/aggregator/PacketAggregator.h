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

#ifndef PACKETAGGREGATOR_H_
#define PACKETAGGREGATOR_H_

#include "Rules.hpp"
#include "BaseAggregator.h"
#include "core/Module.h"
#include "core/Source.h"
#include "core/Destination.h"


#include <pthread.h>


/**
 * does the same as IpfixAggregator, only faster and only for raw packets
 * (IpfixAggregator is mainly used for aggregation of IPFIX flows)
 * inherits functionality of ExpressAggregator
 */
class PacketAggregator
		: public BaseAggregator, public Destination<Packet*>
{
public:
	PacketAggregator(uint32_t pollinterval);
	virtual ~PacketAggregator();

	virtual void receive(Packet* e);

	virtual string getStatisticsXML(double interval);


protected:
	virtual BaseHashtable* createHashtable(Rule* rule, uint16_t minBufferTime,
			uint16_t maxBufferTime, uint8_t hashbits);

private:
	uint32_t statPacketsReceived;
	uint32_t statIgnoredPackets;
};

#endif /*PACKETAGGREGATOR_H_*/
