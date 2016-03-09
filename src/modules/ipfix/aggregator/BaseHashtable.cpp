/*
 * Vermont Aggregator Subsystem
 * Copyright (C) 2009 Vermont Project
 * Copyright (C) 2014 Oliver Gasser
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

#include "BaseHashtable.h"

#include <sstream>
#include <stdint.h>
#include <iostream>

using namespace std;

/**
 * Creates and initializes a new hashtable buffer for flows matching @c rule
 */
BaseHashtable::BaseHashtable(Source<IpfixRecord*>* recordsource, Rule* rule,
		uint16_t minBufferTime, uint16_t maxBufferTime, uint8_t hashbits)
	: biflowAggregation(rule->biflowAggregation),
	  revKeyMapper(NULL),
	  switchArray(NULL),
	  htableBits(hashbits),
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
	  templateDestructionRecordIM("IpfixTemplateDestructionRecord", 0),
	  hbucketIM("BucketListElement", 0),
	  aggInProgress(false)
{
	msg(MSG_INFO, "Hashtable initialized with following parameters:");
	msg(MSG_INFO, "  - minBufferTime=%d", minBufferTime);
	msg(MSG_INFO, "  - maxBufferTime=%d", maxBufferTime);
	msg(MSG_INFO, "  - htableBits=%d", hashbits);

	buckets = new HashtableBucket*[htableSize];
	for (uint32_t i = 0; i < htableSize; i++)
		buckets[i] = NULL;

	createDataTemplate(rule);

	if (biflowAggregation) {
		genBiflowStructs();
	}
}

/**
 * returns value > 0  if given type needs additional private data for aggregation
 * this is included in dataRecords after the "normal" fields
 */
uint32_t BaseHashtable::getPrivateDataLength(const InformationElement::IeInfo& type)
{
	//TODO: if(type.enterprise=29305)... (Gerhard, 12/2009)
	if(type.enterprise & IPFIX_PEN_vermont) {
		switch (type.id) {
			case IPFIX_ETYPEID_frontPayload:
			case IPFIX_ETYPEID_transportOctetDeltaCount:
				return sizeof(PayloadPrivateData);

			case IPFIX_ETYPEID_dpaForcedExport:
				return sizeof(DpaPrivateData);

			case IPFIX_ETYPEID_maxPacketGap: // old flow end time (to calculate packet gap)
				return 8;

			default:
				return 0;
		}
	}
	return 0;
}

void BaseHashtable::createDataTemplate(Rule* rule)
{
	int dataLength = 0; /**< length in bytes of the @c data field */

	dataTemplate.reset(new TemplateInfo);
	dataTemplate->templateId = rule->id;
	// Let's first assume that this is a normal Template
	if(rule->preceding != 0) {
		dataTemplate->preceding = rule->preceding;
		dataTemplate->setId = TemplateInfo::IpfixDataTemplate;
	} else {
		dataTemplate->setId = TemplateInfo::IpfixTemplate;
	}
	fieldLength = 0;

	fieldModifier = (Rule::Field::Modifier*) malloc(rule->fieldCount
			* sizeof(Rule::Field::Modifier));

	for (int32_t i = 0; i < rule->fieldCount; i++) {
		Rule::Field* rf = rule->field[i];

		if (rf->pattern != NULL) {
			// This is a Data Template
			dataTemplate->setId = TemplateInfo::IpfixDataTemplate;
			/* create new fixed-data field containing pattern */
			dataTemplate->dataCount++;
			dataTemplate->dataInfo = (TemplateInfo::FieldInfo*) realloc(dataTemplate->dataInfo,
					sizeof(TemplateInfo::FieldInfo) * dataTemplate->dataCount);
			TemplateInfo::FieldInfo* fi = &dataTemplate->dataInfo[dataTemplate->dataCount - 1];
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
			dataTemplate->fieldInfo = (TemplateInfo::FieldInfo*) realloc(dataTemplate->fieldInfo,
					sizeof(TemplateInfo::FieldInfo) * dataTemplate->fieldCount);
			TemplateInfo::FieldInfo* fi = &dataTemplate->fieldInfo[dataTemplate->fieldCount - 1];
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
	for (uint32_t i = 0; i < dataTemplate->fieldCount; i++) {
		TemplateInfo::FieldInfo* fi = &dataTemplate->fieldInfo[i];
		uint32_t len = getPrivateDataLength(fi->type);
		if (len > 0) {
			fi->privDataOffset = fieldLength + privDataLength;
			privDataLength += len;
		}
		if (fi->type==InformationElement::IeInfo(IPFIX_ETYPEID_frontPayload, IPFIX_PEN_vermont))
			fpLengthOffset = fi->privDataOffset;
		if (fi->type==InformationElement::IeInfo(IPFIX_ETYPEID_frontPayload, IPFIX_PEN_vermont|IPFIX_PEN_reverse))
			revfpLengthOffset = fi->privDataOffset;
	}

	// update private data offsets for fields which access private data from other fields
	// example: front payload length accesses data from front payload
	for (uint32_t i = 0; i < dataTemplate->fieldCount; i++) {
		TemplateInfo::FieldInfo* fi = &dataTemplate->fieldInfo[i];
		if (fi->type==InformationElement::IeInfo(IPFIX_ETYPEID_frontPayloadLen, IPFIX_PEN_vermont)) {
			if (!fpLengthOffset) {
				THROWEXCEPTION("no front payload field specified in template, so front payload length is not available either");
			}
			fi->privDataOffset = fpLengthOffset;
		}
		// we want to access the same private data within these fields as in frontPayload
		if (fi->type==InformationElement::IeInfo(IPFIX_ETYPEID_frontPayloadLen, IPFIX_PEN_vermont|IPFIX_PEN_reverse)) {
			if (!revfpLengthOffset) {
				THROWEXCEPTION("no reverse front payload field specified in template, so front payload length is not available either");
			}
			fi->privDataOffset = revfpLengthOffset;
		}
		// check validity of field
		if (fi->type==InformationElement::IeInfo(IPFIX_ETYPEID_frontPayloadPktCount, IPFIX_PEN_vermont)) {
			if (!fpLengthOffset) {
				THROWEXCEPTION("no front payload field specified in template, so front payload packet count is not available either");
			}
		}
	}
}

/**
 * De-allocates memory of the given hashtable buffer.
 * All remaining Buckets are exported, then destroyed
 */
BaseHashtable::~BaseHashtable()
{
	for (uint32_t i = 0; i < htableSize; i++)
		if (buckets[i] != NULL) {
			HashtableBucket* bucket = buckets[i];
			while (bucket != 0) {
				HashtableBucket* nextBucket = (HashtableBucket*) bucket->next;
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
HashtableBucket* BaseHashtable::createBucket(boost::shared_array<IpfixRecord::Data> data,
		uint32_t obsdomainid, HashtableBucket* next, HashtableBucket* prev, uint32_t hash, uint32_t flowStartTime)
{
	HashtableBucket* bucket = new HashtableBucket();
	bucket->expireTime = flowStartTime + minBufferTime;
	bucket->forceExpireTime = flowStartTime + maxBufferTime;
	bucket->data = data;
	bucket->next = next;
	bucket->prev = prev;
	bucket->hash = hash;
	bucket->observationDomainID = obsdomainid;
	bucket->forceExpiry = false;

	return bucket;
}

/**
 * Exports the given @c bucket
 */
void BaseHashtable::exportBucket(HashtableBucket* bucket)
{
	/* Pass Data Record to exporter interface */
	IpfixDataRecord* ipfixRecord = dataDataRecordIM.getNewInstance();
	ipfixRecord->sourceID = sourceID;
	ipfixRecord->templateInfo = dataTemplate;
	ipfixRecord->dataLength = fieldLength;
	ipfixRecord->message = bucket->data;
	ipfixRecord->data = bucket->data.get();

	recordSource->send(ipfixRecord);

	statRecordsSent++;
}

/**
 * De-allocates memory used by the given @c bucket
 */
void BaseHashtable::destroyBucket(HashtableBucket* bucket)
{
	delete bucket;
}


/**
 * removes given bucket from the hashtable
 */
void BaseHashtable::removeBucket(HashtableBucket* bucket)
{
	if (bucket->next || bucket->prev)
		statMultiEntries--;
	if (!bucket->next && !bucket->prev)
		statEmptyBuckets++;
	if (bucket->prev) {
		bucket->prev->next = bucket->next;
	} else {
		buckets[bucket->hash] = bucket->next;
	}
	if (bucket->next) {
		bucket->next->prev = bucket->prev;
	}
	bucket->next = NULL;
	bucket->prev = NULL;
	bucket->hash = 0;
	bucket->inTable = false;
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

	HashtableBucket* bucket = 0;
	BucketListElement* node = 0;

	if (!exportList.isEmpty) {
		while (exportList.head) { //check the first entry in the BucketList
			node = exportList.head;
			bucket = node->bucket;
			// TODO: change this one list to two lists: one for active, one for passive timeout
			// problem here: flows with active timeout may be exported passive timeout seconds too late
			// now must be updated by the child classes
			if ((bucket->expireTime < now) || (bucket->forceExpireTime < now) || all) {
				if (now > bucket->forceExpireTime) {
					DPRINTF("expireFlows: forced expiry");
				} else if (now > bucket->expireTime) {
					DPRINTF("expireFlows: normal expiry");
				}
				if (bucket->inTable) removeBucket(bucket);
				statExportedBuckets++;
				exportBucket(bucket);
				exportList.remove(node);
				destroyBucket(bucket);
				node->removeReference();
				statTotalEntries--;
			}//end if
			else
				break;
		}//end while
	}

	atomic_release(&aggInProgress);
}

/**
 * Checks whether the given @c type is one of the types that has to be aggregated
 * @return 1 if flow is to be aggregated
 */
int BaseHashtable::isToBeAggregated(InformationElement::IeInfo& type)
{
	switch (type.enterprise) {
		case 0:
			switch (type.id) {
				case IPFIX_TYPEID_flowStartSysUpTime:
				case IPFIX_TYPEID_flowStartSeconds:
				case IPFIX_TYPEID_flowStartMilliseconds:
				case IPFIX_TYPEID_flowStartMicroseconds:
				case IPFIX_TYPEID_flowStartNanoseconds:
				case IPFIX_TYPEID_flowEndSysUpTime:
				case IPFIX_TYPEID_flowEndSeconds:
				case IPFIX_TYPEID_flowEndMilliseconds:
				case IPFIX_TYPEID_flowEndMicroseconds:
				case IPFIX_TYPEID_flowEndNanoseconds:
				case IPFIX_TYPEID_octetTotalCount:
				case IPFIX_TYPEID_octetDeltaCount:
				case IPFIX_TYPEID_postOctetDeltaCount:
				case IPFIX_TYPEID_packetDeltaCount:
				case IPFIX_TYPEID_packetTotalCount:
				case IPFIX_TYPEID_postPacketDeltaCount:
				case IPFIX_TYPEID_droppedOctetDeltaCount:
				case IPFIX_TYPEID_droppedPacketDeltaCount:
				case IPFIX_TYPEID_tcpControlBits:
					return 1;
			}
			break;

		case IPFIX_PEN_reverse:
			switch (type.id) {
				case IPFIX_TYPEID_flowStartSeconds:
				case IPFIX_TYPEID_flowStartMilliseconds:
				case IPFIX_TYPEID_flowStartNanoseconds:
				case IPFIX_TYPEID_flowEndSeconds:
				case IPFIX_TYPEID_flowEndMilliseconds:
				case IPFIX_TYPEID_flowEndNanoseconds:
				case IPFIX_TYPEID_octetDeltaCount:
				case IPFIX_TYPEID_octetTotalCount:
				case IPFIX_TYPEID_packetDeltaCount:
				case IPFIX_TYPEID_packetTotalCount:
				case IPFIX_TYPEID_tcpControlBits:
					return 1;
			}
			break;

		case IPFIX_PEN_vermont:
			switch (type.id) {
				case IPFIX_ETYPEID_frontPayload:
				case IPFIX_ETYPEID_frontPayloadLen:
				case IPFIX_ETYPEID_frontPayloadPktCount:
				case IPFIX_ETYPEID_maxPacketGap:
				case IPFIX_ETYPEID_dpaForcedExport:
				case IPFIX_ETYPEID_dpaFlowCount:
				case IPFIX_ETYPEID_dpaReverseStart:
				case IPFIX_ETYPEID_transportOctetDeltaCount:
					return 1;
			}
			break;

		case IPFIX_PEN_vermont|IPFIX_PEN_reverse:
			switch (type.id) {
				case IPFIX_ETYPEID_frontPayload:
				case IPFIX_ETYPEID_frontPayloadLen:
				case IPFIX_ETYPEID_frontPayloadPktCount:
				case IPFIX_ETYPEID_maxPacketGap:
				case IPFIX_ETYPEID_dpaForcedExport:
				case IPFIX_ETYPEID_dpaFlowCount:
				case IPFIX_ETYPEID_dpaReverseStart:
				case IPFIX_ETYPEID_transportOctetDeltaCount:
					return 1;
			}
			break;
	}
	return 0;
}

/**
 * sends datatemplate to following modules
 */
void BaseHashtable::sendDataTemplate()
{
	IpfixTemplateRecord* ipfixRecord = dataTemplateRecordIM.getNewInstance();
	ipfixRecord->sourceID = sourceID;
	ipfixRecord->templateInfo = dataTemplate;
	recordSource->send(ipfixRecord);
}

/**
 * sends template withdrawal
 */
void BaseHashtable::sendTemplateDestructionRecord()
{
	IpfixTemplateDestructionRecord* ipfixRecord = templateDestructionRecordIM.getNewInstance();
	ipfixRecord->sourceID = sourceID;
	ipfixRecord->templateInfo = dataTemplate;
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
	// we do not need to destroy the template since every module should delete stored templates during reconfiguration and shutdown
	// sendTemplateDestructionRecord();
}

void BaseHashtable::preReconfiguration()
{
	msg(MSG_INFO, "BaseHashtable: Forcing export for flows, then destroy Template.");
	expireFlows(true);
	// we do not need to destroy the template since every module should delete stored templates during reconfiguration
	// sendTemplateDestructionRecord();
}

/**
 * invalidates used template
 */
void BaseHashtable::onReconfiguration1()
{
}

/**
 * recreates data template and sends it to following modules
 */
void BaseHashtable::postReconfiguration()
{
	// send the template again, as this module is still working with the same template
	// after reconfiguration (else this function would not be called)
	sendDataTemplate();
}

std::string BaseHashtable::getStatisticsXML(double interval)
{
	ostringstream oss;
	oss << "<entries>" << statTotalEntries << "</entries>";
	oss << "<emptyBuckets>" << statEmptyBuckets << "</emptyBuckets>";
	oss << "<multientryBuckets>" << statMultiEntries << "</multientryBuckets>";
	uint32_t diff = statExportedBuckets - statLastExpBuckets;
	statLastExpBuckets += diff;
	oss << "<exportedEntries>" << (uint32_t) ((double) diff / interval) << "</exportedEntries>";
	return oss.str();
}

void BaseHashtable::mapReverseElement(const InformationElement::IeInfo& ieinfo)
{
	int i = dataTemplate->getFieldIndex(ieinfo);
	if (i<0)
		THROWEXCEPTION("failed to retrieve IPFIX field id %s (derived as reverse field). Maybe this element is not contained in flow definition?",
				ieinfo.toString().c_str());
	flowReverseMapper.push_back(i);
}


void BaseHashtable::genBiflowStructs()
{
	int32_t srcIPIdx = -1;
	int32_t dstIPIdx = -1;
	int32_t srcPortIdx = -1;
	int32_t dstPortIdx = -1;
	int32_t srcAsIdx = -1;
	int32_t dstAsIdx = -1;
	uint32_t maxFieldSize = 0;


	// search for offsets in dataTemplate
	revKeyMapper = new uint32_t[dataTemplate->fieldCount];
	DPRINTF("fieldCount=%d", dataTemplate->fieldCount);
	for (int32_t i=0; i<dataTemplate->fieldCount; i++) {
		DPRINTF("fieldCount=%d", i);
		TemplateInfo::FieldInfo* fi = &dataTemplate->fieldInfo[i];
		if (fi->type.length>maxFieldSize) maxFieldSize = fi->type.length;
		bool defaultassign = false;
		switch (fi->type.enterprise) {
			case 0:
				switch (fi->type.id) {
					case IPFIX_TYPEID_protocolIdentifier:
					case IPFIX_TYPEID_icmpTypeCodeIPv4:
						mapReverseElement(fi->type);
						break;
					case IPFIX_TYPEID_sourceIPv4Address:
						srcIPIdx = i;
						mapReverseElement(InformationElement::IeInfo(IPFIX_TYPEID_destinationIPv4Address, 0));
						break;
					case IPFIX_TYPEID_destinationIPv4Address:
						dstIPIdx = i;
						mapReverseElement(fi->type);
						break;
					case IPFIX_TYPEID_sourceTransportPort:
						srcPortIdx = i;
						mapReverseElement(InformationElement::IeInfo(IPFIX_TYPEID_destinationTransportPort, 0));
						break;
					case IPFIX_TYPEID_destinationTransportPort:
						dstPortIdx = i;
						mapReverseElement(fi->type);
						break;
					case IPFIX_TYPEID_bgpSourceAsNumber:
						srcAsIdx = i;
						mapReverseElement(InformationElement::IeInfo(IPFIX_TYPEID_bgpSourceAsNumber, 0));
						break;
					case IPFIX_TYPEID_bgpDestinationAsNumber:
						dstAsIdx = i;
						mapReverseElement(InformationElement::IeInfo(IPFIX_TYPEID_bgpDestinationAsNumber, 0));
						break;
					default:
						defaultassign = true;
						break;
				}
				break;
			case IPFIX_PEN_vermont:
				switch (fi->type.id) {
					// do not reverse these fields
					case IPFIX_ETYPEID_dpaForcedExport:
					case IPFIX_ETYPEID_dpaFlowCount:
					case IPFIX_ETYPEID_dpaReverseStart:
					case IPFIX_ETYPEID_anonymisationType:
						mapReverseElement(fi->type);
						break;

					default:
						defaultassign = true;
						break;
				}
				break;
			default:
				defaultassign = true;
				break;
		}

		if (defaultassign) {
			// this call is dangerous, as calculated type ids may not exist at all
			// but mapReverseElement will detect those and throw an exception
			DPRINTF("field %s", fi->type.toString().c_str());
			if ((fi->type.enterprise&IPFIX_PEN_reverse)==0) {
				InformationElement::IeInfo rev = fi->type.getReverseDirection();
				mapReverseElement(rev);
				DPRINTF("mapping field %s to field %s", fi->type.toString().c_str(), rev.toString().c_str());
			} else {
				// do not reverse element
				mapReverseElement(fi->type);
				DPRINTF("not mapping field %s to its reverse element", fi->type.toString().c_str());
			}
		}
	}

	switchArray = new char[maxFieldSize];

	// check if it's possible to obtain the reverse flow
	if ((srcIPIdx<0) || (dstIPIdx<0) || (srcPortIdx<0) || (dstPortIdx<0)) {
		THROWEXCEPTION("no biflow aggregation possible for current template, but was activated in configuration!");
	}

	for (int i=0; i<dataTemplate->fieldCount; i++) {
		TemplateInfo::FieldInfo* fi = &dataTemplate->fieldInfo[i];
		switch (fi->type.id) {
			case IPFIX_TYPEID_sourceIPv4Address:
				revKeyMapper[i] = dstIPIdx;
				break;
			case IPFIX_TYPEID_destinationIPv4Address:
				revKeyMapper[i] = srcIPIdx;
				break;
			case IPFIX_TYPEID_sourceTransportPort:
				revKeyMapper[i] = dstPortIdx;
				break;
			case IPFIX_TYPEID_destinationTransportPort:
				revKeyMapper[i] = srcPortIdx;
				break;
			case IPFIX_TYPEID_bgpSourceAsNumber:
				revKeyMapper[i] = srcAsIdx;
				break;
			case IPFIX_TYPEID_bgpDestinationAsNumber:
				revKeyMapper[i] = dstAsIdx;
				break;
			default:
				revKeyMapper[i] = i;
				break;
		}
	}
}


/**
 * turns a whole flow record inside its bucket around for biflow aggregation
 */
void BaseHashtable::reverseFlowBucket(HashtableBucket* bucket)
{
	for (uint32_t i = 0; i < dataTemplate->fieldCount; i++) {
		TemplateInfo::FieldInfo* fi = &dataTemplate->fieldInfo[i];
		TemplateInfo::FieldInfo* fi2 = &dataTemplate->fieldInfo[flowReverseMapper[i]];

		if (fi != fi2) {
			//msg(MSG_ERROR, "mapping idx %d to idx %d", i, flowReverseMapper[i]);
			//msg(MSG_ERROR, "mapping IE %s to IE %s", fi->type.toString().c_str(), fi2->type.toString().c_str());
			//if (fi->type.id == 152) {
			//	uint64_t oldStart = ntohll(*((uint64_t*)(bucket->data.get() + fi->offset)));
			//	uint64_t newStart = ntohll(*((uint64_t*)(bucket->data.get() + fi2->offset)));
			//	msg(MSG_ERROR, "old: %lu / new: %lu compare: %d", oldStart, newStart, oldStart < newStart);
			//}
			IpfixRecord::Data* src = bucket->data.get()+fi->offset;
			IpfixRecord::Data* dst = bucket->data.get()+fi2->offset;
			uint32_t len = fi->type.length;
			memcpy(switchArray, src, len);
			memcpy(src, dst, len);
			memcpy(dst, switchArray, len);
		}
	}
}
