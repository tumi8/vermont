#ifndef BASEHASHTABLE_H_
#define BASEHASHTABLE_H_

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

#include <stdint.h>

#include "IpfixRecord.hpp"
#include "Rule.hpp"
#include "common/StatisticsManager.h"
#include "reconf/Module.h"


class BaseHashtable : public StatisticsModule
{
public:
	// configuration options for size of hashtable
	// FIXME: should be moved to configuration file
	static const uint32_t HTABLE_BITS = 16;
	static const uint32_t HTABLE_SIZE = 1<<HTABLE_BITS;
	
	/**
	 * Single Bucket containing one buffered flow's variable data.
	 * Is either a direct entry in @c Hashtable::bucket or a member of another Hashtable::Bucket's spillchain
	 */
	class Bucket
	{
	public:
		uint32_t expireTime; /**< timestamp when this bucket will expire if no new flows are added */
		uint32_t forceExpireTime; /**< timestamp when this bucket is forced to expire */
		boost::shared_array<IpfixRecord::Data> data; /**< contains variable fields of aggregated flow; format defined in Hashtable::dataInfo::fieldInfo */
		Bucket* next; /**< next bucket in spill chain */
	};
			
			
	BaseHashtable(Source<IpfixRecord*>* recordsource, Rule* rule, uint16_t minBufferTime, 
			uint16_t maxBufferTime);
	virtual ~BaseHashtable();
	
	virtual std::string getStatistics();
	void expireFlows();
	
protected:
	boost::shared_ptr<IpfixRecord::DataTemplateInfo> dataTemplate; /**< structure describing both variable and fixed fields and containing fixed data */
	Bucket* buckets[HTABLE_SIZE]; /**< array of pointers to hash buckets at start of spill chain. Members are NULL where no entry present */
	
	uint16_t minBufferTime; /**< If for a buffered flow no new aggregatable flows arrive for this many seconds, export it */
	uint16_t maxBufferTime; /**< If a buffered flow was kept buffered for this many seconds, export it */

	uint32_t statRecordsReceived; /**< number of records received from other modules, used for statistics */
	uint32_t statRecordsSent; /**< number records sent to next module, used for statistics */
	uint32_t statTotalEntries; /**< number of entries in hashtable, used for statistics */
	uint32_t statEmptyBuckets; /**< number of empty buckets in hashtable, used for statistics */
	uint32_t statExportedBuckets; /**< number of exported entries/flows, used for statistics */
	uint32_t statLastExpBuckets; /**< last number of exported entries/flows, used for statistics */
	uint32_t statMultiEntries; /**< number of entries in hashtable which are not in a single bucket, used for statistics */
	
	uint16_t fieldLength; /**< length in bytes of all variable-length fields */
	Rule::Field::Modifier* fieldModifier; /**< specifies what modifier to apply to a given field */
	Source<IpfixRecord*>* recordSource; /**< pointer to vermont module which is able to send IpfixRecords */
			
	
	int isToBeAggregated(IpfixRecord::FieldInfo::Type type);
	Bucket* createBucket(boost::shared_array<IpfixRecord::Data> data);
	void exportBucket(Bucket* bucket);
	void destroyBucket(Bucket* bucket);
	
};

#endif /*BASEHASHTABLE_H_*/
