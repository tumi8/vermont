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

#include "IpfixParser.hpp"
#include "Rules.hpp"
#include "Hashtable.hpp"
#include <pthread.h>

/**
 * Represents an Aggregator.
 *
 * Uses Rules and Hashtable to implement an IPFIX Aggregator.
 */
class IpfixAggregator : public FlowSink {
	public:
		IpfixAggregator(char* ruleFile, uint16_t minBufferTime, uint16_t maxBufferTime);
		IpfixAggregator(Rules* rules, uint16_t minBufferTime, uint16_t maxBufferTime);
		~IpfixAggregator();
		void buildAggregator(Rules* rules, uint16_t minBufferTime, uint16_t maxBufferTime);

		void start();
		void stop();

		int onDataRecord(IpfixRecord::SourceID* sourceID, IpfixRecord::TemplateInfo* ti, uint16_t length, IpfixRecord::Data* data);
		int onExpDataRecord(IpfixRecord::SourceID* sourceID, uint16_t length, IpfixRecord::Data* ip_data, IpfixRecord::Data* th_data, int classi);
		int onDataDataRecord(IpfixRecord::SourceID* sourceID, IpfixRecord::DataTemplateInfo* ti, uint16_t length, IpfixRecord::Data* data);

		void poll();

		/**
		 * Add a FlowSink that receives flows we export
		 */
		void addFlowSink(FlowSink* flowSink);

		void stats();

	protected:
		Rules* rules; /**< Set of rules that define the aggregator */
		pthread_mutex_t mutex; /**< Mutex to synchronize and/or pause aggregator */
};

#endif
