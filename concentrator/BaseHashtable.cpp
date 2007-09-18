#include "BaseHashtable.h"

#include <sstream>
#include <stdint.h>

using namespace std;


/**
 * Creates and initializes a new hashtable buffer for flows matching @c rule
 */
BaseHashtable::BaseHashtable(Source<IpfixRecord*>* recordsource, Rule* rule, 
		uint16_t minBufferTime, uint16_t maxBufferTime)
	: minBufferTime(minBufferTime),
	  maxBufferTime(maxBufferTime),
	  statRecordsReceived(0),
	  statRecordsSent(0),
	  statTotalEntries(0),
	  statEmptyBuckets(0),
	  statExportedBuckets(0),
	  statLastExpBuckets(0),
	  recordSource(recordsource)
	  
{
	int dataLength = 0; /**< length in bytes of the @c data field */

	for (uint32_t i = 0; i < HTABLE_SIZE; i++)
		buckets[i] = NULL;

	dataTemplate.reset(new IpfixRecord::DataTemplateInfo);
	dataTemplate->templateId=rule->id;
	dataTemplate->preceding=rule->preceding;
	dataTemplate->fieldCount = 0;
	dataTemplate->fieldInfo = NULL;
	fieldLength = 0;
	dataTemplate->dataCount = 0;
	dataTemplate->dataInfo = NULL;
	dataTemplate->data = NULL;
	dataTemplate->userData = NULL;

	fieldModifier
			= (Rule::Field::Modifier*)malloc(rule->fieldCount * sizeof(Rule::Field::Modifier));

	for (int32_t i = 0; i < rule->fieldCount; i++) {
		Rule::Field* rf = rule->field[i];

		if (rf->pattern != NULL) {
			/* create new fixed-data field containing pattern */
			dataTemplate->dataCount++;
			dataTemplate->dataInfo = (IpfixRecord::FieldInfo*)realloc(dataTemplate->dataInfo,
					sizeof(IpfixRecord::FieldInfo) * dataTemplate->dataCount);
			IpfixRecord::FieldInfo* fi = &dataTemplate->dataInfo[dataTemplate->dataCount - 1];
			fi->type = rf->type;
			fi->offset = dataLength;
			dataLength += fi->type.length;
			dataTemplate->data = (IpfixRecord::Data*)realloc(dataTemplate->data, dataLength);
			memcpy(dataTemplate->data + fi->offset, rf->pattern, fi->type.length);
		}

		if (rf->modifier != Rule::Field::DISCARD) {
			/* define new data field with Rule::Field's type */
			dataTemplate->fieldCount++;
			dataTemplate->fieldInfo = (IpfixRecord::FieldInfo*)realloc(dataTemplate->fieldInfo,
					sizeof(IpfixRecord::FieldInfo) * dataTemplate->fieldCount);
			IpfixRecord::FieldInfo* fi = &dataTemplate->fieldInfo[dataTemplate->fieldCount - 1];
			fi->type = rf->type;
			fi->offset = fieldLength;
			fieldLength += fi->type.length;
			fieldModifier[dataTemplate->fieldCount - 1] = rf->modifier;
		}

	}

	StatisticsManager::getInstance().addModule(this);
}


/**
 * De-allocates memory of the given hashtable buffer.
 * All remaining Buckets are exported, then destroyed
 */
BaseHashtable::~BaseHashtable()
{
	StatisticsManager::getInstance().removeModule(this);

	for (uint32_t i = 0; i < HTABLE_SIZE; i++) if (buckets[i] != NULL) {
		Bucket* bucket = buckets[i];
		while (bucket != 0) {
			Bucket* nextBucket = (Bucket*)bucket->next;
			// we don't want to export the buckets, as the exporter thread may already be shut down!
			//exportBucket(bucket);
			destroyBucket(bucket);
			bucket = nextBucket;
		}
	}

	/* Inform Exporter of Data Template destruction */
	// exporter has already shut down
	/*
	boost::shared_ptr<IpfixDataTemplateDestructionRecord> ipfixRecord(new IpfixDataTemplateDestructionRecord);
	ipfixRecord->sourceID.reset();
	ipfixRecord->dataTemplateInfo = dataTemplate;
	flowSink->push(ipfixRecord);
	push(ipfixRecord);
	 */

	free(fieldModifier);

}

/**
 * Initializes memory for a new bucket in @c ht containing @c data
 */
BaseHashtable::Bucket* BaseHashtable::createBucket(boost::shared_array<IpfixRecord::Data> data) 
{
	Bucket* bucket = new Bucket();
	bucket->expireTime = time(0) + minBufferTime;
	bucket->forceExpireTime = time(0) + maxBufferTime;
	bucket->data = data;
	bucket->next = 0;

	return bucket;
}


/**
 * Exports the given @c bucket
 */
void BaseHashtable::exportBucket(BaseHashtable::Bucket* bucket) 
{
	/* Pass Data Record to exporter interface */
	IpfixDataDataRecord* ipfixRecord = new IpfixDataDataRecord();
	ipfixRecord->sourceID.reset();
	ipfixRecord->dataTemplateInfo = dataTemplate;
	ipfixRecord->dataLength = fieldLength;
	ipfixRecord->message = bucket->data;
	ipfixRecord->data = bucket->data.get();
	
	recordSource->send(ipfixRecord);

	statRecordsSent++;
}


/**
 * De-allocates memory used by the given @c bucket
 */
void BaseHashtable::destroyBucket(BaseHashtable::Bucket* bucket) 
{
	delete bucket;
}


/**
 * Exports all expired flows and removes them from the buffer
 */
void BaseHashtable::expireFlows() {

	uint32_t now = time(0);

	uint32_t noEntries = 0;
	uint32_t emptyBuckets = 0;
	uint32_t exportedBuckets = 0;
	uint32_t multiEntries = 0;
	/* check each hash bucket's spill chain */
	for (uint32_t i = 0; i < HTABLE_SIZE; i++) {
		if (buckets[i] != 0) {
			Bucket* bucket = buckets[i];
			Bucket* pred = 0;

			/* iterate over spill chain */
			bool firstentry = true;
			while (bucket != 0) {
				if (firstentry) firstentry = false;
				else multiEntries++;
				noEntries++;
				Bucket* nextBucket = (Bucket*)bucket->next;
				if ((now > bucket->expireTime) || (now > bucket->forceExpireTime)) {
					if(now > bucket->forceExpireTime)  DPRINTF("expireFlows: forced expiry");
					else if(now > bucket->expireTime)  DPRINTF("expireFlows: normal expiry");

					exportedBuckets++;
					exportBucket(bucket);
					destroyBucket(bucket);
					if (pred) {
						pred->next = nextBucket;
					} else {
						buckets[i] = nextBucket;
					}
				} else {
					pred = bucket;
				}

				bucket = nextBucket;
			}
		} else {
			emptyBuckets++;
		}
	} 	

	statTotalEntries = noEntries;
	statEmptyBuckets = emptyBuckets;
	statExportedBuckets += exportedBuckets;
	statMultiEntries = multiEntries;
}


/**
 * Checks whether the given @c type is one of the types that has to be aggregated
 * @return 1 if flow is to be aggregated
 */
int BaseHashtable::isToBeAggregated(IpfixRecord::FieldInfo::Type type)
{
	switch (type.id) {
		case IPFIX_TYPEID_flowStartSysUpTime:
		case IPFIX_TYPEID_flowStartSeconds:
		case IPFIX_TYPEID_flowStartMilliSeconds:
		case IPFIX_TYPEID_flowStartMicroSeconds:
		case IPFIX_TYPEID_flowStartNanoSeconds:
			return 1;

		case IPFIX_TYPEID_flowEndSysUpTime:
		case IPFIX_TYPEID_flowEndSeconds:
		case IPFIX_TYPEID_flowEndMilliSeconds:
		case IPFIX_TYPEID_flowEndMicroSeconds:
		case IPFIX_TYPEID_flowEndNanoSeconds:
			return 1;

		case IPFIX_TYPEID_octetDeltaCount:
		case IPFIX_TYPEID_postOctetDeltaCount:
		case IPFIX_TYPEID_packetDeltaCount:
		case IPFIX_TYPEID_postPacketDeltaCount:
		case IPFIX_TYPEID_droppedOctetDeltaCount:
		case IPFIX_TYPEID_droppedPacketDeltaCount:
			return 1;

		case IPFIX_TYPEID_octetTotalCount:
		case IPFIX_TYPEID_packetTotalCount:
		case IPFIX_TYPEID_droppedOctetTotalCount:
		case IPFIX_TYPEID_droppedPacketTotalCount:
		case IPFIX_TYPEID_postMCastPacketDeltaCount:
		case IPFIX_TYPEID_postMCastOctetDeltaCount:
		case IPFIX_TYPEID_observedFlowTotalCount:
		case IPFIX_TYPEID_exportedOctetTotalCount:
		case IPFIX_TYPEID_exportedMessageTotalCount:
		case IPFIX_TYPEID_exportedFlowTotalCount:
			DPRINTF("isToBeAggregated: Will not aggregate %s field", typeid2string(type.id));
			return 0;

		default:
			return 0;
	}
}


std::string BaseHashtable::getStatistics()
{
	ostringstream oss;
	oss << "Hashtable: number of hashtable entries      : " << statTotalEntries << endl;
	oss << "Hashtable: number of empty hashtable buckets: " << statEmptyBuckets << endl;
	oss << "Hashtable: number of hashtable buckets with multiple entries: " << statMultiEntries << endl;
	uint32_t diff = statExportedBuckets - statLastExpBuckets;
	statLastExpBuckets += diff;
	oss << "Hashtable: number of exported entries       : " << diff << endl;
	return oss.str();
}
