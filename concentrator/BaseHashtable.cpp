#include "BaseHashtable.h"

#include <sstream>
#include <stdint.h>

using namespace std;


/**
 * Creates and initializes a new hashtable buffer for flows matching @c rule
 */
BaseHashtable::BaseHashtable(Source<IpfixRecord*>* recordsource, Rule* rule,
		uint16_t minBufferTime, uint16_t maxBufferTime, uint8_t hashbits)
	: htableBits(hashbits),
	  htableSize(1<<hashbits),
	  minBufferTime(minBufferTime),
	  maxBufferTime(maxBufferTime),
	  statRecordsReceived(0),
	  statRecordsSent(0),
	  statTotalEntries(0),
	  statEmptyBuckets(0),
	  statExportedBuckets(0),
	  statLastExpBuckets(0),
	  statMultiEntries(0),
	  fieldModifier(0),
	  recordSource(recordsource),
	  sourceID(new IpfixRecord::SourceID),
	  dataDataRecordIM("IpfixDataDataRecord", 0),
	  dataTemplateRecordIM("IpfixDataTemplateRecord", 0),
	  aggInProgress(false),
	  resendTemplate(true)
{
	msg(MSG_INFO, "Hashtable initialized with following parameters:");
	msg(MSG_INFO, "  - minBufferTime=%d", minBufferTime);
	msg(MSG_INFO, "  - maxBufferTime=%d", maxBufferTime);
	msg(MSG_INFO, "  - htableBits=%d", hashbits);

	memset(sourceID.get(), 0, sizeof(IpfixRecord::SourceID));

	buckets = new Bucket*[htableSize];
	for (uint32_t i = 0; i < htableSize; i++)
		buckets[i] = NULL;

	createDataTemplate(rule);

}

/**
 * returns value > 0  if given type needs additional private data for aggregation
 * this is included in dataRecords after the "normal" fields
 */
uint32_t BaseHashtable::getPrivateDataLength(IpfixRecord::FieldInfo::Type type)
{
	switch (type.id) {
		case IPFIX_ETYPEID_frontPayload:		// four bytes TCP sequence ID, four bytes for byte-counter for aggregated data
		case IPFIX_ETYPEID_revFrontPayload:		// "
		case IPFIX_ETYPEID_maxPacketGap:		// old flow end time (to calculate packet gap)
		case IPFIX_ETYPEID_revMaxPacketGap:		// old flow end time (to calculate packet gap)
			return 8;

		default:
			return 0;
	}

	return 0;
}

void BaseHashtable::createDataTemplate(Rule* rule)
{
	int dataLength = 0; /**< length in bytes of the @c data field */

	dataTemplate.reset(new IpfixRecord::DataTemplateInfo);
	dataTemplate->templateId=rule->id;
	dataTemplate->preceding=rule->preceding;
	dataTemplate->fieldCount = 0;
	dataTemplate->fieldInfo = NULL;
	fieldLength = 0;
	dataTemplate->dataCount = 0;
	dataTemplate->dataInfo = NULL;
	dataTemplate->data = NULL;
	dataTemplate->dataLength = 0;
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
			fi->privDataOffset = 0;
			dataLength += fi->type.length;
			dataTemplate->dataLength = dataLength;
			dataTemplate->data = (IpfixRecord::Data*)realloc(dataTemplate->data, dataLength);
			memcpy(dataTemplate->data + fi->offset, rf->pattern, fi->type.length);
		}
		/* gerhard: If we have a pattern (and fixed value field), the variable value field is implicitely discarded.
		 * Note: This is necessary because of the double meaning/usage of Rule::Field.type.length:
		 * If a pattern is present, this variable holds the length of the pattern (or fixed length field)
		 * and not the length of the normal field holding a single value. As a consequence, we cannot use
		 * Rule.Field.type.length as length of the variable value field.
		 * If someone really wants to enable the export of both, pattern and variable value field of the same
		 * type, then he has to remove the double meaning/usage (or set the variable field length to the
		 * default value for the specific type).
		 */
		else if (rf->modifier != Rule::Field::DISCARD) {
			/* define new data field with Rule::Field's type */
			dataTemplate->fieldCount++;
			dataTemplate->fieldInfo = (IpfixRecord::FieldInfo*)realloc(dataTemplate->fieldInfo,
					sizeof(IpfixRecord::FieldInfo) * dataTemplate->fieldCount);
			IpfixRecord::FieldInfo* fi = &dataTemplate->fieldInfo[dataTemplate->fieldCount - 1];
			fi->type = rf->type;
			fi->offset = fieldLength;
			fi->privDataOffset = 0;
			fieldLength += fi->type.length;
			fieldModifier[dataTemplate->fieldCount - 1] = rf->modifier;
		}
	}

	// add private data offsets for fields
	uint32_t fpLengthOffset = 0;
	uint32_t revfpLengthOffset = 0;
	privDataLength = 0;
	for (uint32_t i=0; i<dataTemplate->fieldCount; i++) {
		IpfixRecord::FieldInfo* fi = &dataTemplate->fieldInfo[i];
		uint32_t len = getPrivateDataLength(fi->type);
		if (len>0) {
			fi->privDataOffset = fieldLength+privDataLength;
			privDataLength += len;
		}
		if (fi->type.id == IPFIX_ETYPEID_frontPayload)
			fpLengthOffset = fi->privDataOffset + 4;
		if (fi->type.id == IPFIX_ETYPEID_revFrontPayload)
			revfpLengthOffset = fi->privDataOffset + 4;
	}

	// update private data offsets for fields which access private data from other fields
	// example: front payload length accesses data from front payload
	for (uint32_t i=0; i<dataTemplate->fieldCount; i++) {
		IpfixRecord::FieldInfo* fi = &dataTemplate->fieldInfo[i];
		if (fi->type.id==IPFIX_ETYPEID_frontPayloadLen) {
			if (!fpLengthOffset) {
				THROWEXCEPTION("no front payload field specified in template, so front payload length is not available either");
			}
			fi->privDataOffset = fpLengthOffset;
		}
		if (fi->type.id == IPFIX_ETYPEID_revFrontPayloadLen) {
			if (!revfpLengthOffset) {
				THROWEXCEPTION("no reverse front payload field specified in template, so front payload length is not available either");
			}
			fi->privDataOffset = revfpLengthOffset;
		}
	}
}


/**
 * De-allocates memory of the given hashtable buffer.
 * All remaining Buckets are exported, then destroyed
 */
BaseHashtable::~BaseHashtable()
{
	for (uint32_t i = 0; i < htableSize; i++) if (buckets[i] != NULL) {
		Bucket* bucket = buckets[i];
		while (bucket != 0) {
			Bucket* nextBucket = (Bucket*)bucket->next;
			// we don't want to export the buckets, as the exporter thread may already be shut down!
			//exportBucket(bucket);
			destroyBucket(bucket);
			bucket = nextBucket;
		}
	}

	delete[] buckets;
	free(fieldModifier);

}

/**
 * Initializes memory for a new bucket in @c ht containing @c data
 */
BaseHashtable::Bucket* BaseHashtable::createBucket(boost::shared_array<IpfixRecord::Data> data, uint32_t obsdomainid)
{
	Bucket* bucket = new Bucket();
	bucket->expireTime = time(0) + minBufferTime;
	bucket->forceExpireTime = time(0) + maxBufferTime;
	bucket->data = data;
	bucket->next = 0;
	bucket->observationDomainID = obsdomainid;

	return bucket;
}


/**
 * Exports the given @c bucket
 */
void BaseHashtable::exportBucket(BaseHashtable::Bucket* bucket)
{
	/* Pass Data Record to exporter interface */
	IpfixDataDataRecord* ipfixRecord = dataDataRecordIM.getNewInstance();
	ipfixRecord->sourceID = sourceID;
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
void BaseHashtable::expireFlows(bool all)
{
	// the following lock should almost never fail (only during reconfiguration)
	while (atomic_lock(&aggInProgress)) {
		timespec req;
		req.tv_sec = 0;
		req.tv_nsec = 50000000;
		nanosleep(&req, &req);
	}

	uint32_t now = time(0);

	uint32_t noEntries = 0;
	uint32_t emptyBuckets = 0;
	uint32_t exportedBuckets = 0;
	uint32_t multiEntries = 0;

	if (resendTemplate) {
		sendDataTemplate();
		resendTemplate = false;
	}


	/* check each hash bucket's spill chain */
	for (uint32_t i = 0; i < htableSize; i++) {
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
				if ((now > bucket->expireTime) || (now > bucket->forceExpireTime) || all) {
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

	atomic_release(&aggInProgress);
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
		case IPFIX_TYPEID_flowEndSysUpTime:
		case IPFIX_TYPEID_flowEndSeconds:
		case IPFIX_TYPEID_flowEndMilliSeconds:
		case IPFIX_TYPEID_flowEndMicroSeconds:
		case IPFIX_TYPEID_flowEndNanoSeconds:
		case IPFIX_TYPEID_octetDeltaCount:
		case IPFIX_TYPEID_postOctetDeltaCount:
		case IPFIX_TYPEID_packetDeltaCount:
		case IPFIX_TYPEID_postPacketDeltaCount:
		case IPFIX_TYPEID_droppedOctetDeltaCount:
		case IPFIX_TYPEID_droppedPacketDeltaCount:
		case IPFIX_TYPEID_tcpControlBits:
		case IPFIX_ETYPEID_frontPayload:
		case IPFIX_ETYPEID_frontPayloadLen:
		case IPFIX_ETYPEID_revFrontPayload:
		case IPFIX_ETYPEID_revFrontPayloadLen:
		case IPFIX_ETYPEID_revFlowStartSeconds:
		case IPFIX_ETYPEID_revFlowStartMilliSeconds:
		case IPFIX_ETYPEID_revFlowStartNanoSeconds:
		case IPFIX_ETYPEID_revFlowEndSeconds:
		case IPFIX_ETYPEID_revFlowEndMilliSeconds:
		case IPFIX_ETYPEID_revFlowEndNanoSeconds:
		case IPFIX_ETYPEID_revOctetDeltaCount:
		case IPFIX_ETYPEID_revPacketDeltaCount:
		case IPFIX_ETYPEID_revTcpControlBits:
		case IPFIX_ETYPEID_maxPacketGap:
		case IPFIX_ETYPEID_revMaxPacketGap:
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


/**
 * sends datatemplate to following modules
 */
void BaseHashtable::sendDataTemplate()
{
	IpfixDataTemplateRecord* ipfixRecord = dataTemplateRecordIM.getNewInstance();
	ipfixRecord->sourceID.reset();
	ipfixRecord->dataTemplateInfo = dataTemplate;
	recordSource->send(ipfixRecord);
}

/**
 * sends the generated template to all following modules
 */
void BaseHashtable::performStart()
{
	sendDataTemplate();
}

/**
 * invalidates used template
 */
void BaseHashtable::performShutdown()
{
	// this tells all modules that the template should not be used any more
	dataTemplate.get()->destroyed = true;
}

void BaseHashtable::preReconfiguration()
{
	msg(MSG_INFO, "forcing export for flows");
	expireFlows(true);
}

/**
 * invalidates used template
 */
void BaseHashtable::onReconfiguration1()
{
	// this tells all modules that the template should not be used any more
	dataTemplate.get()->destroyed = true;
}

/**
 * recreates data template and sends it to following modules
 */
void BaseHashtable::postReconfiguration()
{
	// "de-invalidates" the template again, as this module is still working with the same template
	// after reconfiguration (else this function would not be called)
	dataTemplate.get()->destroyed = false;
}


std::string BaseHashtable::getStatisticsXML(double interval)
{
	ostringstream oss;
	oss << "<entries>" << statTotalEntries << "</entries>";
	oss << "<emptyBuckets>" << statEmptyBuckets << "</emptyBuckets>";
	oss << "<multientryBuckets>" << statMultiEntries << "</multientryBuckets>";
	uint32_t diff = statExportedBuckets - statLastExpBuckets;
	statLastExpBuckets += diff;
	oss << "<exportedEntries>" << (uint32_t)((double)diff/interval) << "</exportedEntries>";
	return oss.str();
}
