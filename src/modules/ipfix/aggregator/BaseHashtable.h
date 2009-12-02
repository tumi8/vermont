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

#include "modules/ipfix/IpfixRecord.hpp"
#include "HashtableBuckets.h"
#include "Rule.hpp"
#include "core/Module.h"
#include "common/Sensor.h"
#include "common/atomic_lock.h"


class BaseHashtable : public Sensor
{
public:

	BaseHashtable(Source<IpfixRecord*>* recordsource, Rule* rule, uint16_t minBufferTime,
			uint16_t maxBufferTime, uint8_t hashbits);

	virtual ~BaseHashtable();

	virtual std::string getStatisticsXML(double interval);
	void expireFlows(bool all = false);

	/**
	 * this method is called from the aggregator when the module is started
	 */
	void performStart();

	/**
	 * this method is called from aggregator when module is shut down
	 */
	void performShutdown();

	/**
	 * before reconfiguration this function is called by aggregator
	 */
	void preReconfiguration();

	/**
	 * during reconfiguration this function is called by aggregator
	 */
	void onReconfiguration1();

	/**
	 * after the reconfiguration the aggregator calls this function
	 */
	void postReconfiguration();

protected:
	boost::shared_ptr<TemplateInfo> dataTemplate; /**< structure describing both variable and fixed fields and containing fixed data */
	HashtableBucket** buckets; /**< array of pointers to hash buckets at start of spill chain. Members are NULL where no entry present */

	uint32_t htableBits;
	uint32_t htableSize;

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
	uint16_t privDataLength; /**< length in bytes of all private data fields that are not exported with IPFIX but needed with flow record for aggregation */
	Rule::Field::Modifier* fieldModifier; /**< specifies what modifier to apply to a given field */
	Source<IpfixRecord*>* recordSource; /**< pointer to vermont module which is able to send IpfixRecords */
	boost::shared_ptr<IpfixRecord::SourceID> sourceID; /**< used for hack: we *must* supply an observationDomainID, so take a static one */

	HashtableBucketList exportList;

	InstanceManager<IpfixDataDataRecord> dataDataRecordIM;
	InstanceManager<IpfixTemplateRecord> dataTemplateRecordIM;
	InstanceManager<BucketListElement> hbucketIM;

	alock_t aggInProgress; /** indicates if currently an element is aggregated in the hashtable, used for atomic lock for preReconfiguration */

	bool resendTemplate; /**< set to true if template needs to be sent again */

	int isToBeAggregated(const InformationElement::IeInfo& type);
	HashtableBucket* createBucket(boost::shared_array<IpfixRecord::Data> data, uint32_t obsdomainid,
		HashtableBucket* next, HashtableBucket* prev, uint32_t hash);
	void exportBucket(HashtableBucket* bucket);
	void destroyBucket(HashtableBucket* bucket);
	void createDataTemplate(Rule* rule);
	void sendDataTemplate();
	uint32_t getPrivateDataLength(const InformationElement::IeInfo& type);

};

#endif /*BASEHASHTABLE_H_*/
