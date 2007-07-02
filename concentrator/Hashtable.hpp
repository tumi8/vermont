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

#ifndef HASHING_H
#define HASHING_H

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <boost/smart_ptr.hpp>
#include "FlowSource.hpp"
#include "IpfixParser.hpp"
#include "Rules.hpp"
#include <list>

#define HASHTABLE_SIZE 65536

/**
 * Hash-powered buffer for outgoing flows.
 * This is where outbound flows are aggregated while waiting to be exported.
 *
 * The Hashtable module receives flows from higher levels, 
 * collects them in Buffers, then passes them on to lower levels by calling the 
 * appropriate callback functions.
 *
 * Flows that differ only in aggregatable fields (like @c IPFIX_TYPEID_inOctetDeltaCount) are 
 * aggregated.
 * If for a buffered flow no new aggregatable flows arrive for a certain timespan 
 * or the flow was kept buffered for a certain amount of time it is
 * passed on to lower levels (i.e. exported) and removed from the hashtable.
 *
 * Polling for expired flows is accomplished by periodically calling @c expireFlows().
 *
 * Each @c Hashtable contains some fixed-value IPFIX fields @c Hashtable.data 
 * described by the @c Hashtable.dataInfo array. The remaining, variable-value 
 * fields are stored in @c Hashtable.buckets[].data structures described by the
 * @c Hashtable.fieldInfo array.
 */
class Hashtable : public FlowSource {
	public:
		class Bucket;
		/**
		 * Single Bucket containing one buffered flow's variable data.
		 * Is either a direct entry in @c Hashtable::bucket or a member of another Hashtable::Bucket's spillchain
		 */
		class Bucket {
			public:
				uint32_t expireTime; /**< timestamp when this bucket will expire if no new flows are added */
				uint32_t forceExpireTime; /**< timestamp when this bucket is forced to expire */
				boost::shared_array<IpfixRecord::Data> data; /**< contains variable fields of aggregated flow; format defined in Hashtable::dataInfo::fieldInfo */
				Hashtable::Bucket* next; /**< next bucket in spill chain */
		};

		Hashtable(Rule* rule, uint16_t minBufferTime, uint16_t maxBufferTime);
		~Hashtable();

		int isToBeAggregated(IpfixRecord::FieldInfo::Type type);

		virtual void addFlowSink(FlowSink* flowSink);

		void aggregateTemplateData(IpfixRecord::TemplateInfo* ti, IpfixRecord::Data* data);
		void ExpAggregateTemplateData(const Packet* p);
		void aggregateDataTemplateData(IpfixRecord::DataTemplateInfo* ti, IpfixRecord::Data* data);

		void expireFlows();


		int bucketCount; /**< size of this hashtable (must be HASHTABLE_SIZE) */
		Hashtable::Bucket* buckets[HASHTABLE_SIZE]; /**< array of pointers to hash buckets at start of spill chain. Members are NULL where no entry present */

		int recordsReceived; /**< Statistics: Number of records received from higher-level modules */
		int recordsSent; /**< Statistics: Number of records sent to lower-level modules */

		uint16_t minBufferTime; /**< If for a buffered flow no new aggregatable flows arrive for this many seconds, export it */
		uint16_t maxBufferTime; /**< If a buffered flow was kept buffered for this many seconds, export it */

	protected:

		boost::shared_ptr<IpfixRecord::DataTemplateInfo> dataTemplate; /**< structure describing both variable and fixed fields and containing fixed data */
		uint16_t fieldLength; /**< length in bytes of all variable-length fields */
		Rule::Field::Modifier* fieldModifier; /**< specifies what modifier to apply to a given field */

		Hashtable::Bucket* createBucket(boost::shared_array<IpfixRecord::Data> data);
		void exportBucket(Hashtable::Bucket* bucket);
		void destroyBucket(Hashtable::Bucket* bucket);
		int aggregateField(IpfixRecord::FieldInfo::Type* type, IpfixRecord::Data* baseData, IpfixRecord::Data* deltaData);
		int aggregateFlow(IpfixRecord::Data* baseFlow, IpfixRecord::Data* flow);
		uint16_t getHash(IpfixRecord::Data* data);
		int equalFlow(IpfixRecord::Data* flow1, IpfixRecord::Data* flow2);
		void bufferDataBlock(boost::shared_array<IpfixRecord::Data> data);
};
	
#endif
