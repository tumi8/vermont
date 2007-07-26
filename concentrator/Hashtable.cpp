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

#include "crc16.hpp"

#include "Hashtable.hpp"

#include "common/msg.h"

#include <string.h>
#include <netinet/in.h>
#include <time.h>
#include <sstream>


using namespace std;

/**
 * Initializes memory for a new bucket in @c ht containing @c data
 */
Hashtable::Bucket* Hashtable::createBucket(boost::shared_array<IpfixRecord::Data> data) {
	Hashtable::Bucket* bucket = new Hashtable::Bucket;
	bucket->expireTime = time(0) + minBufferTime;
	bucket->forceExpireTime = time(0) + maxBufferTime;
	bucket->data = data;
	bucket->next = 0;

	return bucket;
}

/**
 * Exports the given @c bucket
 */
void Hashtable::exportBucket(Hashtable::Bucket* bucket) {
	/* Pass Data Record to exporter interface */
	boost::shared_ptr<IpfixDataDataRecord> ipfixRecord(new IpfixDataDataRecord);
	ipfixRecord->sourceID.reset();
	ipfixRecord->dataTemplateInfo = dataTemplate;
	ipfixRecord->dataLength = fieldLength;
	ipfixRecord->message = bucket->data;
	ipfixRecord->data = bucket->data.get();
	push(ipfixRecord);

	recordsSent++;
}

/**
 * De-allocates memory used by the given @c bucket
 */
void Hashtable::destroyBucket(Hashtable::Bucket* bucket) {
	delete bucket;
}

/**
 * Creates and initializes a new hashtable buffer for flows matching @c rule
 */
Hashtable::Hashtable(Rule* rule, uint16_t minBufferTime, uint16_t maxBufferTime) {
	int i;
	int dataLength = 0; /**< length in bytes of the @c data field */

	this->minBufferTime = minBufferTime;
	this->maxBufferTime = maxBufferTime;

	bucketCount = HASHTABLE_SIZE;
	for (i = 0; i < bucketCount; i++) buckets[i] = NULL;

	recordsReceived = 0;
	recordsSent = 0;
	statTotalEntries = 0;
	statEmptyBuckets = 0;

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

	fieldModifier = (Rule::Field::Modifier*)malloc(rule->fieldCount * sizeof(Rule::Field::Modifier));

	for (i = 0; i < rule->fieldCount; i++) {
		Rule::Field* rf = rule->field[i];

		if (rf->pattern != NULL) {
			/* create new fixed-data field containing pattern */
			dataTemplate->dataCount++;
			dataTemplate->dataInfo = (IpfixRecord::FieldInfo*)realloc(dataTemplate->dataInfo, sizeof(IpfixRecord::FieldInfo) * dataTemplate->dataCount);
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
			dataTemplate->fieldInfo = (IpfixRecord::FieldInfo*)realloc(dataTemplate->fieldInfo, sizeof(IpfixRecord::FieldInfo) * dataTemplate->fieldCount);
			IpfixRecord::FieldInfo* fi = &dataTemplate->fieldInfo[dataTemplate->fieldCount - 1];
			fi->type = rf->type;
			fi->offset = fieldLength;
			fieldLength += fi->type.length;
			fieldModifier[dataTemplate->fieldCount - 1] = rf->modifier;
		}

	}

	/* Informing the Exporter of a new Data Template is done when adding the callback functions */

	buildExpHelperTable();
	StatisticsManager::getInstance().addModule(this);
}

/**
 * De-allocates memory of the given hashtable buffer.
 * All remaining Buckets are exported, then destroyed
 */
Hashtable::~Hashtable() {
	int i;
	for (i = 0; i < HASHTABLE_SIZE; i++) if (buckets[i] != NULL) {
		Hashtable::Bucket* bucket = buckets[i];
		while (bucket != 0) {
			Hashtable::Bucket* nextBucket = (Hashtable::Bucket*)bucket->next;
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

	// free express aggregator helper structures
	delete[] expHelperTable.expFieldData;
	delete[] expHelperTable.varSrcPtrFields;
}

/** 
 * copy functions which were extracted from ExpcopyData
 * those copy data from the original raw packet into the ipfix bucket in the hashtable
 * (always called, when a new bucket has to be created for a new flow)
 */
void Hashtable::copyDataEqualLengthNoMod(IpfixRecord::Data* dst, const IpfixRecord::Data* src, ExpFieldData* efd)
{
	memcpy(dst, src, efd->srcLength);
}
void Hashtable::copyDataGreaterLengthIPNoMod(IpfixRecord::Data* dst, const IpfixRecord::Data* src, ExpFieldData* efd)
{
	bzero(dst+efd->srcLength, efd->dstLength-efd->srcLength);
	memcpy(dst, src, efd->srcLength);
}
void Hashtable::copyDataGreaterLengthIPMask(IpfixRecord::Data* dst, const IpfixRecord::Data* src, ExpFieldData* efd)
{
	// attention: length MUST be 5 for destination!
	*reinterpret_cast<uint32_t*>(dst) = *reinterpret_cast<const uint32_t*>(src);
	// copy mask information into 5th byte
	dst[4] = efd->data;
}
void Hashtable::copyDataGreaterLengthNoMod(IpfixRecord::Data* dst, const IpfixRecord::Data* src, ExpFieldData* efd)
{
	bzero(dst, efd->dstLength-efd->srcLength);
	memcpy(dst+efd->dstLength-efd->srcLength, src, efd->srcLength);
}
void Hashtable::copyDataSetOne(IpfixRecord::Data* dst, const IpfixRecord::Data* src, ExpFieldData* efd)
{
	bzero(dst, efd->dstLength);
	// set last byte of array to one (network byte order!)
	dst[efd->dstLength-1] = 1;
}


/**
 * helper function for buildExpHelperTable
 * does some error checking on given parameters and returns function which is appropriate
 * to copy field in flow
 */
void (*Hashtable::getCopyDataFunction(const ExpFieldData* efd))(IpfixRecord::Data*, const IpfixRecord::Data*, ExpFieldData*)
{
	// some error handling
	if (efd->modifier == Rule::Field::DISCARD) {
		THROWEXCEPTION("tried to copy data with field modifier set to discard");
	} else if ((efd->modifier != Rule::Field::KEEP) && (efd->modifier != Rule::Field::AGGREGATE) &&
			   (efd->modifier < Rule::Field::MASK_START) && (efd->modifier > Rule::Field::MASK_END)) {
		THROWEXCEPTION("unknown modifier %d", efd->modifier);
	}
	switch (efd->typeId) {
		case IPFIX_TYPEID_protocolIdentifier:
		case IPFIX_TYPEID_tcpControlBits:
			if (efd->dstLength != 1) {
				THROWEXCEPTION("unsupported length %d for type %d", efd->dstLength, efd->typeId);
			}
			break;

		case IPFIX_TYPEID_sourceTransportPort:
		case IPFIX_TYPEID_destinationTransportPort:
			if (efd->dstLength != 2) {
				THROWEXCEPTION("unsupported length %d for type %d", efd->dstLength, efd->typeId);
			}
			break;

		case IPFIX_TYPEID_flowStartSysUpTime:
		case IPFIX_TYPEID_flowStartSeconds:
		case IPFIX_TYPEID_flowStartMicroSeconds:
		case IPFIX_TYPEID_flowStartNanoSeconds:
			if (efd->dstLength != 4) {
				THROWEXCEPTION("unsupported length %d for type %d", efd->dstLength, efd->typeId);
			}
			break;

		case IPFIX_TYPEID_flowStartMilliSeconds:
			if (efd->dstLength != 8) {
				THROWEXCEPTION("unsupported length %d for type %d", efd->dstLength, efd->typeId);
			}
			break;

		case IPFIX_TYPEID_flowEndSysUpTime:
		case IPFIX_TYPEID_flowEndSeconds:
		case IPFIX_TYPEID_flowEndMicroSeconds:
		case IPFIX_TYPEID_flowEndNanoSeconds:
			if (efd->dstLength != 4) {
				THROWEXCEPTION("unsupported length %d for type %d", efd->dstLength, efd->typeId);
			}
			break;

		case IPFIX_TYPEID_flowEndMilliSeconds:
			if (efd->dstLength != 8) {
				THROWEXCEPTION("unsupported length %d for type %d", efd->dstLength, efd->typeId);
			}
			break;

		case IPFIX_TYPEID_octetDeltaCount:
			if (efd->dstLength != 8) {
				THROWEXCEPTION("unsupported length %d for type %d", efd->dstLength, efd->typeId);
			}

		case IPFIX_TYPEID_packetDeltaCount:
			switch (efd->dstLength) {
				case 1:
				case 2:
				case 4:
				case 8:
					break;
					
				default:
					THROWEXCEPTION("unsupported length %d for type %d", efd->dstLength, efd->typeId);
					break;
			}
			break;

		case IPFIX_TYPEID_sourceIPv4Address:
		case IPFIX_TYPEID_destinationIPv4Address:
			if (efd->dstLength != 5) {
					THROWEXCEPTION("unsupported length %d for type %d", efd->dstLength, efd->typeId);
			}
			break;

		default:
			THROWEXCEPTION("non-aggregatable type: %d", efd->typeId);
			break;
	}

	// now decide on the correct copy function
	if (efd->dstLength == efd->srcLength) {
		return copyDataEqualLengthNoMod;
	} else if (efd->dstLength > efd->srcLength) {
		if (efd->typeId == IPFIX_TYPEID_sourceIPv4Address || efd->typeId == IPFIX_TYPEID_destinationIPv4Address) {
			if ((efd->modifier >= Rule::Field::MASK_START) && (efd->modifier <= Rule::Field::MASK_END)) {
				if (efd->dstLength != 5) {
					THROWEXCEPTION("destination data size must be 5, but is %d - need to store mask in there", efd->dstLength);
				}
				return copyDataGreaterLengthIPMask;
			} else {
				return copyDataGreaterLengthIPNoMod;
			}
		} else if (efd->typeId == IPFIX_TYPEID_packetDeltaCount) {
			return copyDataSetOne;
		} else {
			return copyDataGreaterLengthNoMod;
		}
	} else {
		THROWEXCEPTION("target buffer too small. Expected buffer %s of length %d, got one with length %d", typeid2string(efd->typeId), efd->srcLength, efd->dstLength);
	}

	THROWEXCEPTION("this line should never be reached");
	return 0;
}


/**
 * helper function for buildExpHelperTable
 */
void Hashtable::fillExpFieldData(ExpFieldData* efd, IpfixRecord::FieldInfo* hfi, Rule::Field::Modifier fieldModifier)
{
	efd->typeId = hfi->type.id;
	efd->dstIndex = hfi->offset;
	efd->srcLength = IpfixRecord::TemplateInfo::getFieldLength(hfi->type);
	efd->dstLength = hfi->type.length;
	efd->modifier = fieldModifier;
	efd->varSrcIdx = IpfixRecord::TemplateInfo::isRawPacketPtrVariable(hfi->type);
	efd->copyDataFunc = getCopyDataFunction(efd);

	// initialize static source index, if current field does not have a variable pointer
	if (!efd->varSrcIdx) {
		// create temporary packet just for initializing our optimization structure
		Packet p;
		efd->srcIndex = IpfixRecord::TemplateInfo::getRawPacketFieldIndex(hfi->type.id, &p);
	}
}

/**
 * builds internal structure expHelperTable for fast aggregation of raw packets
 * used in the express aggregator
 */
void Hashtable::buildExpHelperTable()
{
	expHelperTable.expFieldData = new ExpFieldData[dataTemplate->fieldCount];
	expHelperTable.varSrcPtrFields = new uint16_t[dataTemplate->fieldCount];

	// at first, fill data structure with aggregatable fields
	uint16_t efdIdx = 0;
	for (int i=0; i<dataTemplate->fieldCount; i++) {
		IpfixRecord::FieldInfo* hfi = &dataTemplate->fieldInfo[i];
		if (!isToBeAggregated(hfi->type)) continue;
		ExpFieldData* efd = &expHelperTable.expFieldData[efdIdx++];
		fillExpFieldData(efd, hfi, fieldModifier[i]);
	}
	expHelperTable.noAggFields = efdIdx;

	// now the remaining fields
	for (int i=0; i<dataTemplate->fieldCount; i++) {
		IpfixRecord::FieldInfo* hfi = &dataTemplate->fieldInfo[i];
		if (isToBeAggregated(hfi->type)) continue;
		ExpFieldData* efd = &expHelperTable.expFieldData[efdIdx++];
		fillExpFieldData(efd, hfi, fieldModifier[i]);
	}

	// fill structure which contains field with variable pointers
	int noVarFields = 0;
	for (int i=0; i<dataTemplate->fieldCount; i++) {
		if (expHelperTable.expFieldData[i].varSrcIdx) expHelperTable.varSrcPtrFields[noVarFields++] = i;
	}
	expHelperTable.varSrcPtrFieldsLen = noVarFields;

}

/**
 * Exports all expired flows and removes them from the buffer
 */
void Hashtable::expireFlows() {
	uint32_t now = time(0);
	int i;

	uint32_t noEntries = 0;
	uint32_t emptyBuckets = 0;
	/* check each hash bucket's spill chain */
	for (i = 0; i < bucketCount; i++) {
		if (buckets[i] != 0) {
			Hashtable::Bucket* bucket = buckets[i];
			Hashtable::Bucket* pred = 0;

			/* iterate over spill chain */
			while (bucket != 0) {
				noEntries++;
				Hashtable::Bucket* nextBucket = (Hashtable::Bucket*)bucket->next;
				if ((now > bucket->expireTime) || (now > bucket->forceExpireTime)) {
					if(now > bucket->forceExpireTime)  DPRINTF("expireFlows: forced expiry");
					else if(now > bucket->expireTime)  DPRINTF("expireFlows: normal expiry");

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
}

/**
 * Returns the sum of two uint32_t values in network byte order
 */
uint64_t addUint64Nbo(uint64_t i, uint64_t j) {
	return (htonll(ntohll(i) + ntohll(j)));
}

/**
 * Returns the sum of two uint32_t values in network byte order
 */
uint32_t addUint32Nbo(uint32_t i, uint32_t j) {
	return (htonl(ntohl(i) + ntohl(j)));
}

/**
 * Returns the sum of two uint16_t values in network byte order
 */
uint16_t addUint16Nbo(uint16_t i, uint16_t j) {
	return (htons(ntohs(i) + ntohs(j)));
}

/**
 * Returns the sum of two uint8_t values in network byte order.
 * As if we needed this...
 */
uint8_t addUint8Nbo(uint8_t i, uint8_t j) {
	return (i + j);
}
	
/**
 * Returns the lesser of two uint32_t values in network byte order
 */
uint32_t lesserUint32Nbo(uint32_t i, uint32_t j) {
	return (ntohl(i) < ntohl(j))?(i):(j);
}

/**
 * Returns the greater of two uint32_t values in network byte order
 */
uint32_t greaterUint32Nbo(uint32_t i, uint32_t j) {
	return (ntohl(i) > ntohl(j))?(i):(j);
}

/**
 * Returns the lesser of two uint64_t values in network byte order
 */
uint64_t lesserUint64Nbo(uint64_t i, uint64_t j) {
	return (ntohll(i) < ntohll(j))?(i):(j);
}

/**
 * Returns the greater of two uint64_t values in network byte order
 */
uint64_t greaterUint64Nbo(uint64_t i, uint64_t j) {
	return (ntohll(i) > ntohll(j))?(i):(j);
}

/**
 * Checks whether the given @c type is one of the types that has to be aggregated
 * @return 1 if flow is to be aggregated
 */
int Hashtable::isToBeAggregated(IpfixRecord::FieldInfo::Type type)
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

/**
 * Adds (or otherwise aggregates) @c deltaData to @c baseData
 */
int Hashtable::aggregateField(IpfixRecord::FieldInfo::Type* type, IpfixRecord::Data* baseData, IpfixRecord::Data* deltaData) {
	switch (type->id) {

		case IPFIX_TYPEID_flowStartSysUpTime:
		case IPFIX_TYPEID_flowStartSeconds:
		case IPFIX_TYPEID_flowStartMicroSeconds:
		case IPFIX_TYPEID_flowStartNanoSeconds:
			if (type->length != 4) {
				DPRINTF("unsupported length %d for type %d", type->length, type->id);
				goto out;
			}

			*(uint32_t*)baseData = lesserUint32Nbo(*(uint32_t*)baseData, *(uint32_t*)deltaData);
			break;

		case IPFIX_TYPEID_flowStartMilliSeconds:
			if (type->length != 8) {
				DPRINTF("unsupported length %d for type %d", type->length, type->id);
				goto out;
			}

			*(uint64_t*)baseData = lesserUint64Nbo(*(uint64_t*)baseData, *(uint64_t*)deltaData);
			break;

		case IPFIX_TYPEID_flowEndSysUpTime:
		case IPFIX_TYPEID_flowEndSeconds:
		case IPFIX_TYPEID_flowEndMicroSeconds:
		case IPFIX_TYPEID_flowEndNanoSeconds:
			if (type->length != 4) {
				DPRINTF("unsupported length %d for type %d", type->length, type->id);
				goto out;
			}

			*(uint32_t*)baseData = greaterUint32Nbo(*(uint32_t*)baseData, *(uint32_t*)deltaData);
			break;

		case IPFIX_TYPEID_flowEndMilliSeconds:
			if (type->length != 8) {
				DPRINTF("unsupported length %d for type %d", type->length, type->id);
				goto out;
			}

			*(uint64_t*)baseData = greaterUint64Nbo(*(uint64_t*)baseData, *(uint64_t*)deltaData);
			break;

		case IPFIX_TYPEID_octetDeltaCount:
		case IPFIX_TYPEID_postOctetDeltaCount:
		case IPFIX_TYPEID_packetDeltaCount:
		case IPFIX_TYPEID_postPacketDeltaCount:
		case IPFIX_TYPEID_droppedOctetDeltaCount:
		case IPFIX_TYPEID_droppedPacketDeltaCount:
			// TODO: tobi_optimize
			// converting all values to network byte order when sending ipfix packets would be much faster
			switch (type->length) {
				case 1:
					*(uint8_t*)baseData = addUint8Nbo(*(uint8_t*)baseData, *(uint8_t*)deltaData);
					return 0;
				case 2:
					*(uint16_t*)baseData = addUint16Nbo(*(uint16_t*)baseData, *(uint16_t*)deltaData);
					return 0;
				case 4:
					*(uint32_t*)baseData = addUint32Nbo(*(uint32_t*)baseData, *(uint32_t*)deltaData);
					return 0;
				case 8:
					*(uint64_t*)baseData = addUint64Nbo(*(uint64_t*)baseData, *(uint64_t*)deltaData);
					return 0;
				default:
					DPRINTF("unsupported length %d for type %d", type->length, type->id);
					goto out;
			}
			break;

		default:
			DPRINTF("non-aggregatable type: %d", type->id);
			goto out;
			break;
	}

	return 0;
out:
	return 1;
}

/**
 * Adds (or otherwise aggregates) pertinent fields of @c flow to @c baseFlow
 */
int Hashtable::aggregateFlow(IpfixRecord::Data* baseFlow, IpfixRecord::Data* flow)
{
	int i;

	if(!baseFlow) {
		DPRINTF("aggregateFlow: baseFlow is NULL");
		return 1;
	}

	if(!flow){
		DPRINTF("aggregateFlow: flow is NULL");
		return 1;
	}

	for (i = 0; i < dataTemplate->fieldCount; i++) {
		IpfixRecord::FieldInfo* fi = &dataTemplate->fieldInfo[i];

		if(!isToBeAggregated(fi->type)) {
			continue;
		}
		aggregateField(&fi->type, baseFlow + fi->offset, flow + fi->offset);
	}

	return 0;
}

/**
 * Returns a hash value corresponding to all variable, non-aggregatable fields of a flow
 */
uint16_t Hashtable::getHash(IpfixRecord::Data* data) {
	int i;

	uint16_t hash = 0;
	for (i = 0; i < dataTemplate->fieldCount; i++) {
		if(isToBeAggregated(dataTemplate->fieldInfo[i].type)) {
			continue;
		}
		hash = crc16(hash,
			     dataTemplate->fieldInfo[i].type.length,
			     (char*)data + dataTemplate->fieldInfo[i].offset
			    );
	}

	return hash;
}

/**
 * Checks if two data fields are binary equal
 * @return 1 if fields are equal
 */
int equalRaw(IpfixRecord::FieldInfo::Type* data1Type, IpfixRecord::Data* data1, IpfixRecord::FieldInfo::Type* data2Type, IpfixRecord::Data* data2) {
	int i;

	if(data1Type->id != data2Type->id) return 0;
	if(data1Type->length != data2Type->length) return 0;
	if(data1Type->eid != data2Type->eid) return 0;

	for(i = 0; i < data1Type->length; i++) {
		if(data1[i] != data2[i]) {
			return 0;
		}
	}

	return 1;
}

/**
 * Checks if all of two flows' (non-aggregatable) fields are binary equal
 * @return 1 if fields are equal
 */
int Hashtable::equalFlow(IpfixRecord::Data* flow1, IpfixRecord::Data* flow2)
{
	int i;

	if(flow1 == flow2) return 1;

	for(i = 0; i < dataTemplate->fieldCount; i++) {
		IpfixRecord::FieldInfo* fi = &dataTemplate->fieldInfo[i];

		if(isToBeAggregated(fi->type)) {
			continue;
		}

		if(!equalRaw(&fi->type, flow1 + fi->offset, &fi->type, flow2 + fi->offset)) {
			return 0;
		}
	}

	return 1;
}

/**
 * Inserts a data block into the hashtable
 */
void Hashtable::bufferDataBlock(boost::shared_array<IpfixRecord::Data> data)
{
	recordsReceived++;

	uint16_t hash = getHash(data.get());
	Hashtable::Bucket* bucket = buckets[hash];

	if (bucket == 0) {
		/* This slot is still free, place the bucket here */
		DPRINTF("bufferDataBlock: creating bucket\n");
		buckets[hash] = createBucket(data);
		return;
	}

	/* This slot is already used, search spill chain for equal flow */
	while(1) {
		if (equalFlow(bucket->data.get(), data.get())) {
			DPRINTF("appending to bucket\n");

			aggregateFlow(bucket->data.get(), data.get());
			// TODO: tobi_optimize
			// replace call of time() with access to a static variable which is updated regularly (such as every 100ms)
			bucket->expireTime = time(0) + minBufferTime;

			break;
		}

		if (bucket->next == 0) {
			DPRINTF("creating bucket\n");

			bucket->next = createBucket(data);
			break;
		}
		bucket = (Hashtable::Bucket*)bucket->next;
	}
}

/**
 * Copies \c srcData to \c dstData applying \c modifier.
 * Takes care to pad \c srcData with zero-bytes in case it is shorter than \c dstData.
 */
void copyData(IpfixRecord::FieldInfo::Type* dstType, IpfixRecord::Data* dstData, IpfixRecord::FieldInfo::Type* srcType, IpfixRecord::Data* srcData, Rule::Field::Modifier modifier)
{
	if((dstType->id != srcType->id) || (dstType->eid != srcType->eid)) {
		DPRINTF("copyData: Tried to copy field to destination of different type\n");
		return;
	}

	/* Copy data, care for length differences */
	if(dstType->length == srcType->length) {
		memcpy(dstData, srcData, srcType->length);

	} else if(dstType->length > srcType->length) {

		/* TODO: We simply pad with zeroes - will this always be correct? */
		if((dstType->id == IPFIX_TYPEID_sourceIPv4Address) || (dstType->id == IPFIX_TYPEID_destinationIPv4Address)) {
			/* Fields of type IPv4Address-type are padded on the right */
			bzero(dstData, dstType->length);
			memcpy(dstData, srcData, srcType->length);
		} else {
			/* TODO: all other types are padded on the left, i.e. the "big" end */
			bzero(dstData, dstType->length);
			memcpy(dstData + dstType->length - srcType->length, srcData, srcType->length);
		}

	} else {
		DPRINTF("Target buffer too small. Buffer expected %s of length %d, got one with length %dn", typeid2string(dstType->id), srcType->length, dstType->length);
		return;
	}

	/* Apply modifier */
	if(modifier == Rule::Field::DISCARD) {
		DPRINTF("Tried to copy data w/ having field modifier set to discard\n");
		return;
	} else if((modifier == Rule::Field::KEEP) || (modifier == Rule::Field::AGGREGATE)) {

	} else if((modifier >= Rule::Field::MASK_START) && (modifier <= Rule::Field::MASK_END)) {

		if((dstType->id != IPFIX_TYPEID_sourceIPv4Address) && (dstType->id != IPFIX_TYPEID_destinationIPv4Address)) {
			DPRINTF("Tried to apply mask to %s field\n", typeid2string(dstType->id));
			return;
		}

		if (dstType->length != 5) {
			DPRINTF("Destination data to short - no room to store mask\n");
			return;
		}

		uint8_t imask = 32 - (modifier - (int)Rule::Field::MASK_START);
		dstData[4] = imask; /* store the inverse network mask */

		if (imask > 0) {
			if (imask == 8) {
				dstData[3] = 0x00;
			} else if (imask == 16) {
				dstData[2] = 0x00;
				dstData[3] = 0x00;
			} else if (imask == 24) {
				dstData[1] = 0x00;
				dstData[2] = 0x00;
				dstData[3] = 0x00;
			} else if (imask == 32) {
				dstData[0] = 0x00;
				dstData[1] = 0x00;
				dstData[2] = 0x00;
				dstData[3] = 0x00;
			} else {
				int pattern = 0;
				int i;
				for(i = 0; i < imask; i++) {
					pattern |= (1 << i);
				}

				*(uint32_t*)dstData = htonl(ntohl(*(uint32_t*)(dstData)) & ~pattern);
			}
		}

	} else {
		DPRINTF("Unhandled field modifier: %d\n", modifier);
		return;
	}
}

void ExpcopyData(const IpfixRecord::FieldInfo::Type* dstType, IpfixRecord::Data* dstData, const IpfixRecord::FieldInfo::Type* srcType, const IpfixRecord::Data* srcData, const Rule::Field::Modifier modifier)
{
	if((dstType->id != srcType->id)) {
		DPRINTF("copyData: Tried to copy field to destination of different type\n");
		return;
	}

	/* Copy data, care for length differences */
	if(dstType->length == srcType->length) {
		memcpy(dstData, srcData, srcType->length);

	} else if(dstType->length > srcType->length) {

		/* TODO: We simply pad with zeroes - will this always be correct? */
		if((dstType->id == IPFIX_TYPEID_sourceIPv4Address) || (dstType->id == IPFIX_TYPEID_destinationIPv4Address)) {
			/* Fields of type IPv4Address-type are padded on the right */
			bzero(dstData, dstType->length);
			memcpy(dstData, srcData, srcType->length);
		} else {
			/* TODO: all other types are padded on the left, i.e. the "big" end */
			bzero(dstData, dstType->length);
			memcpy(dstData + dstType->length - srcType->length, srcData, srcType->length);
		}

	} else {
		DPRINTF("Target buffer too small. Buffer expected %s of length %d, got one with length %d\n", typeid2string(dstType->id), dstType->length, srcType->length);
		return;
	}

	/* Apply modifier */
	if(modifier == Rule::Field::DISCARD) {
		DPRINTF("Tried to copy data w/ having field modifier set to discard\n");
		return;
	} else if((modifier == Rule::Field::KEEP) || (modifier == Rule::Field::AGGREGATE)) {

	} else if((modifier >= Rule::Field::MASK_START) && (modifier <= Rule::Field::MASK_END)) {

		if((dstType->id != IPFIX_TYPEID_sourceIPv4Address) && (dstType->id != IPFIX_TYPEID_destinationIPv4Address)) {
			DPRINTF("Tried to apply mask to %s field\n", typeid2string(dstType->id));
			return;
		}

		if (dstType->length != 5) {
			DPRINTF("Destination data to short - no room to store mask\n");
			return;
		}

		uint8_t imask = 32 - (modifier - (int)Rule::Field::MASK_START);
		dstData[4] = imask; /* store the inverse network mask */

		if (imask > 0) {
			if (imask == 8) {
				dstData[3] = 0x00;
			} else if (imask == 16) {
				dstData[2] = 0x00;
				dstData[3] = 0x00;
			} else if (imask == 24) {
				dstData[1] = 0x00;
				dstData[2] = 0x00;
				dstData[3] = 0x00;
			} else if (imask == 32) {
				dstData[0] = 0x00;
				dstData[1] = 0x00;
				dstData[2] = 0x00;
				dstData[3] = 0x00;
			} else {
				int pattern = 0;
				int i;
				for(i = 0; i < imask; i++) {
					pattern |= (1 << i);
				}

				*(uint32_t*)dstData = htonl(ntohl(*(uint32_t*)(dstData)) & ~pattern);
			}
		}

	} else {
		DPRINTF("Unhandled field modifier: %d\n", modifier);
		return;
	}
}



/**
 * Buffer passed flow in Hashtable @c ht
 */
void Hashtable::aggregateTemplateData(IpfixRecord::TemplateInfo* ti, IpfixRecord::Data* data)
{
        DPRINTF("Hashtable::aggregateTemplateData called");
	int i;

	/* Create data block to be inserted into buffer... */
	boost::shared_array<IpfixRecord::Data> htdata(new IpfixRecord::Data[fieldLength]);

	for (i = 0; i < dataTemplate->fieldCount; i++) {
		IpfixRecord::FieldInfo* hfi = &dataTemplate->fieldInfo[i];
		IpfixRecord::FieldInfo* tfi = ti->getFieldInfo(&hfi->type);

		if(!tfi) {
			DPRINTF("Flow to be buffered did not contain %s field\n", typeid2string(hfi->type.id));
			continue;
		}

		DPRINTFL(MSG_VDEBUG, "copyData for type %d, offset %x, starting from pointer %X", tfi->type.id, tfi->offset, data+tfi->offset);
		DPRINTFL(MSG_VDEBUG, "copyData to offset %X", hfi->offset);
		copyData(&hfi->type, htdata.get() + hfi->offset, &tfi->type, data + tfi->offset, fieldModifier[i]);

		/* copy associated mask, should there be one */
		switch (hfi->type.id) {
			case IPFIX_TYPEID_sourceIPv4Address:
				tfi = ti->getFieldInfo(IPFIX_TYPEID_sourceIPv4Mask, 0);

			if(tfi) {
				if(hfi->type.length != 5) {
					DPRINTF("Tried to set mask of length %d IP address\n", hfi->type.length);
				} else {
					if(tfi->type.length == 1) {
						*(uint8_t*)(htdata.get() + hfi->offset + 4) = *(uint8_t*)(data + tfi->offset);
					} else {
						DPRINTF("Cannot process associated mask with invalid length %d\n", tfi->type.length);
					}
				}
			}
			break;

		case IPFIX_TYPEID_destinationIPv4Address:
			tfi = ti->getFieldInfo(IPFIX_TYPEID_destinationIPv4Mask, 0);

			if(tfi) {
				if(hfi->type.length != 5) {
					DPRINTF("Tried to set mask of length %d IP address\n", hfi->type.length);
				} else {
					if(tfi->type.length == 1) {
						*(uint8_t*)(htdata.get() + hfi->offset + 4) = *(uint8_t*)(data + tfi->offset);
					} else {
						DPRINTF("Cannot process associated mask with invalid length %d\n", tfi->type.length);
					}
				}
			}
			break;

		default:
			break;
		}
	}

	/* ...then buffer it */
	bufferDataBlock(htdata);
}

/**
 * calculates hash for given raw packet data in express aggregator
 */
uint16_t Hashtable::expCalculateHash(const IpfixRecord::Data* data)
{
	uint16_t hash = 0;
	for (int i=expHelperTable.noAggFields; i<dataTemplate->fieldCount; i++) {
		ExpFieldData* efd = &expHelperTable.expFieldData[i];
		hash = crc16(hash, efd->srcLength, reinterpret_cast<const char*>(data)+efd->srcIndex);
	}
	return hash;
}

/**
 * copies data from raw packet to a bucket which will be inserted into the hashtable
 * for aggregation (part of express aggregator)
 */
boost::shared_array<IpfixRecord::Data> Hashtable::buildBucketData(const Packet* p)
{
	// new field for insertion into hashtable
	boost::shared_array<IpfixRecord::Data> htdata(new IpfixRecord::Data[fieldLength]);
	IpfixRecord::Data* data = htdata.get();
	
	// copy all data ...
	for (int i=0; i<dataTemplate->fieldCount; i++) {
		ExpFieldData* efd = &expHelperTable.expFieldData[i];
		efd->copyDataFunc(&data[efd->dstIndex], reinterpret_cast<const uint8_t*>(p->netHeader)+efd->srcIndex, efd);
	}

	return htdata;
}

/**
 * aggregates the given field of the raw packet data into a hashtable bucket
 * (part of express aggregator)
 */
void Hashtable::expAggregateField(const ExpFieldData* efd, IpfixRecord::Data* baseData, const IpfixRecord::Data* deltaData)
{
	switch (efd->typeId) {
		case IPFIX_TYPEID_flowStartSeconds:
			*(uint32_t*)baseData = lesserUint32Nbo(*(uint32_t*)baseData, *(uint32_t*)deltaData);
			break;

		case IPFIX_TYPEID_flowStartMilliSeconds:
			*(uint64_t*)baseData = lesserUint64Nbo(*(uint64_t*)baseData, *(uint64_t*)deltaData);
			break;

		case IPFIX_TYPEID_flowEndSeconds:
			*(uint32_t*)baseData = greaterUint32Nbo(*(uint32_t*)baseData, *(uint32_t*)deltaData);
			break;

		case IPFIX_TYPEID_flowEndMilliSeconds:
			*(uint64_t*)baseData = greaterUint64Nbo(*(uint64_t*)baseData, *(uint64_t*)deltaData);
			break;

		case IPFIX_TYPEID_octetDeltaCount: // 8 byte dst, 2 byte src
			*(uint64_t*)baseData = htonll(ntohll(*(uint64_t*)baseData) + ntohs(*(uint16_t*)deltaData));
			break;

		case IPFIX_TYPEID_packetDeltaCount: // 8 byte dst, no src
			*(uint64_t*)baseData = htonll(ntohll(*(uint64_t*)baseData)+1);
			break;

		case IPFIX_TYPEID_tcpControlBits:  // 1 byte src and dst, bitwise-or flows
			*(uint8_t*)baseData |= *(uint8_t*)deltaData;
			break;

			// no other types needed, as this is only for raw field input

		default:
			DPRINTF("non-aggregatable type: %d", efd->typeId);
			break;
	}
}

/**
 * aggregates the given raw packet data into the hashtable bucket
 * (part of express aggregator)
 */
void Hashtable::expAggregateFlow(IpfixRecord::Data* bucket, const Packet* p)
{
	// TODO: tobi_optimize
	// here all fields marked as 'aggregatable' are processed.
	// maybe we should only process fields which are really aggregatable, as e.g. dstIp or srcIp
	// fields will noch be aggregated, as method aggregateField does not do anything with those
	for (int i=0; i<expHelperTable.noAggFields; i++) {
		ExpFieldData* efd = &expHelperTable.expFieldData[i];

		expAggregateField(efd, bucket+efd->dstIndex, p->netHeader+efd->srcIndex);
	}
}

/**
 * compares if given hashtable bucket data is equal with raw packet data
 * (part of express aggregator)
 * @returns true if equal, false if not equal
 */
bool Hashtable::expEqualFlow(IpfixRecord::Data* bucket, const Packet* p)
{
	for (int i=expHelperTable.noAggFields; i<dataTemplate->fieldCount; i++) {
		ExpFieldData* efd = &expHelperTable.expFieldData[i];

		// just compare srcLength bytes, as we still have our original packet data
		if (memcmp(bucket+efd->dstIndex, p->netHeader+efd->srcIndex, efd->srcLength)!=0)
			return false;
	}
	return true;
}

/**
 * masks ip addresses inside raw packet and creates a mask field
 * (part of express aggregator)
 */
void Hashtable::createMaskedField(IpfixRecord::Data* address, uint8_t* ipMask, Rule::Field::Modifier modifier)
{
	uint8_t imask = 32 - (modifier - (int)Rule::Field::MASK_START);
	*ipMask = imask; /* store the inverse network mask */

	if (imask > 0) {
		if (imask == 8) {
			address[3] = 0x00;
		} else if (imask == 16) {
			address[2] = 0x00;
			address[3] = 0x00;
		} else if (imask == 24) {
			address[1] = 0x00;
			address[2] = 0x00;
			address[3] = 0x00;
		} else if (imask == 32) {
			address[0] = 0x00;
			address[1] = 0x00;
			address[2] = 0x00;
			address[3] = 0x00;
		} else {
			int pattern = 0;
			int i;
			for(i = 0; i < imask; i++) {
				pattern |= (1 << i);
			}
			*(uint32_t*)address = htonl(ntohl(*(uint32_t*)(address)) & ~pattern);
		}
	}
}

/**
 * masks ip addresses inside the raw packet if desired
 * additional mask information (is 5th byte in aggregated data) is in ExpFieldData->data
 */
void Hashtable::createMaskedFields(const Packet* p)
{
	if (expHelperTable.dstIpEFieldIndex > 0) {
		ExpFieldData* efd = &expHelperTable.expFieldData[expHelperTable.dstIpEFieldIndex];
		// const_cast: yes, we know that it is not allowed. this is an exception. honestly.
		createMaskedField(const_cast<unsigned char*>(p->netHeader)+efd->srcIndex, &efd->data, efd->modifier);
	}
	if (expHelperTable.srcIpEFieldIndex > 0) {
		ExpFieldData* efd = &expHelperTable.expFieldData[expHelperTable.srcIpEFieldIndex];
		createMaskedField(const_cast<unsigned char*>(p->netHeader)+efd->srcIndex, &efd->data, efd->modifier);
	}
}

/**
 * updates variable pointers to the raw packet data for each packet
 * (part of express aggregator)
 */
void Hashtable::updatePointers(const Packet* p)
{
	for (int i=0; i<expHelperTable.varSrcPtrFieldsLen; i++) {
		ExpFieldData* efd = &expHelperTable.expFieldData[expHelperTable.varSrcPtrFields[i]];
		efd->srcIndex = IpfixRecord::TemplateInfo::getRawPacketFieldIndex(efd->typeId, p);
	}
}

/**
 * Replacement for ExpAggregateTemplateData
 * TODO: write doc!
 * ATTENTION: 
 *  - this function expects not to be called in parallel, as it uses internal buffers which are
 *    *NOT* thread-safe
 *  - raw packet data is modified according to specified masks
 *  - hashes are calculated based on raw packet (masks are already applied then)
 */
void Hashtable::aggregatePacket(const Packet* p)
{
	updatePointers(p);
	createMaskedFields(p);

	uint16_t hash = expCalculateHash(p->netHeader);

	// search bucket inside hashtable
	Hashtable::Bucket* bucket = buckets[hash];
	if (bucket == 0) {
		// slot is free, place bucket there
		DPRINTF("creating new bucket");
		buckets[hash] = createBucket(buildBucketData(p));
		return;
	}

	// This slot is already used, search spill chain for equal flow
	while(1) {
		if (expEqualFlow(bucket->data.get(), p)) {
			DPRINTF("appending to bucket\n");

			expAggregateFlow(bucket->data.get(), p);

			// TODO: tobi_optimize
			// replace call of time() with access to a static variable which is updated regularly (such as every 100ms)
			bucket->expireTime = time(0) + minBufferTime;

			break;
		}

		if (bucket->next == 0) {
			DPRINTF("creating bucket\n");

			bucket->next = createBucket(buildBucketData(p));
			break;
		}
		bucket = (Hashtable::Bucket*)bucket->next;
	}
}

/**
 * Buffer passed flow for Express aggregator
 */
void Hashtable::ExpAggregateTemplateData(const Packet* p)
{
	
	/* Create data block to be inserted into buffer... */
	boost::shared_array<IpfixRecord::Data> htdata(new IpfixRecord::Data[fieldLength]);

	// tobi_optimize:
	// here we copy stuff from the raw packet into the aggregated data structure
	// in many cases this would not be necessary, as the flow has already been added
	// to the hashtable and inside bufferDataBlock, the already copied data is copied into
	// the existing flow and htdata is freed again (-> unnecessary new, copy and delete)
	for (int i = 0; i < dataTemplate->fieldCount; i++) {
		IpfixRecord::FieldInfo* hfi = &dataTemplate->fieldInfo[i];
		const IpfixRecord::Data* tfi = p->netHeader + IpfixRecord::TemplateInfo::getRawPacketFieldIndex(hfi->type.id, p);
		IpfixRecord::FieldInfo fi;

		fi.type.id = hfi->type.id;
		fi.type.length = IpfixRecord::TemplateInfo::getFieldLength(hfi->type);

		if(!tfi) {
			DPRINTF("Flow to be buffered did not contain %s field\n", typeid2string(hfi->type.id));
			continue;
		}

		ExpcopyData(&hfi->type, htdata.get() + hfi->offset, &fi.type, tfi, fieldModifier[i]);
	}

	bufferDataBlock(htdata);
}


/**
 * Buffer passed flow (containing fixed-value fields) in Hashtable @c ht
 */
void Hashtable::aggregateDataTemplateData(IpfixRecord::DataTemplateInfo* ti, IpfixRecord::Data* data)
{
	DPRINTF("called");
	int i;

	/* Create data block to be inserted into buffer... */
	boost::shared_array<IpfixRecord::Data> htdata(new IpfixRecord::Data[fieldLength]);

	for (i = 0; i < dataTemplate->fieldCount; i++) {
		IpfixRecord::FieldInfo* hfi = &dataTemplate->fieldInfo[i];

		/* Copy from matching variable field, should it exist */
		IpfixRecord::FieldInfo* tfi = ti->getFieldInfo(&hfi->type);
		if(tfi) {
			copyData(&hfi->type, htdata.get() + hfi->offset, &tfi->type, data + tfi->offset, fieldModifier[i]);

			/* copy associated mask, should there be one */
			switch (hfi->type.id) {

			case IPFIX_TYPEID_sourceIPv4Address:
				tfi = ti->getFieldInfo(IPFIX_TYPEID_sourceIPv4Mask, 0);
				if(tfi) {
					if(hfi->type.length != 5) {
						DPRINTF("Tried to set mask of length %d IP address\n", hfi->type.length);
					} else {
						if(tfi->type.length == 1) {
							*(uint8_t*)(htdata.get() + hfi->offset + 4) = *(uint8_t*)(data + tfi->offset);
						} else {
							DPRINTF("Cannot process associated mask with invalid length %d\n", tfi->type.length);
						}
					}
				}
				break;

			case IPFIX_TYPEID_destinationIPv4Address:
				tfi = ti->getFieldInfo(IPFIX_TYPEID_destinationIPv4Mask, 0);
				if(tfi) {
					if(hfi->type.length != 5) {
						DPRINTF("Tried to set mask of length %d IP address", hfi->type.length);
					} else {
						if(tfi->type.length == 1) {
							*(uint8_t*)(htdata.get() + hfi->offset + 4) = *(uint8_t*)(data + tfi->offset);
						} else {
							DPRINTF("Cannot process associated mask with invalid length %d", tfi->type.length);
						}
					}
				}
				break;

			default:
				break;
			}

			continue;
		}

		/* No matching variable field. Copy from matching fixed field, should it exist */
		tfi = ti->getDataInfo(&hfi->type);
		if(tfi) {
			copyData(&hfi->type, htdata.get() + hfi->offset, &tfi->type, ti->data + tfi->offset, fieldModifier[i]);

			/* copy associated mask, should there be one */
			switch (hfi->type.id) {

			case IPFIX_TYPEID_sourceIPv4Address:
				tfi = ti->getDataInfo(IPFIX_TYPEID_sourceIPv4Mask, 0);
				if(tfi) {
					if(hfi->type.length != 5) {
						DPRINTF("Tried to set mask of length %d IP address\n", hfi->type.length);
					} else {
						if(tfi->type.length == 1) {
							*(uint8_t*)(htdata.get() + hfi->offset + 4) = *(uint8_t*)(ti->data + tfi->offset);
						} else {
							DPRINTF("Cannot process associated mask with invalid length %d\n", tfi->type.length);
						}
					}
				}
				break;

			case IPFIX_TYPEID_destinationIPv4Address:
				tfi = ti->getDataInfo(IPFIX_TYPEID_destinationIPv4Mask, 0);
				if(tfi) {
					if(hfi->type.length != 5) {
						DPRINTF("Tried to set mask of length %d IP address\n", hfi->type.length);
					} else {
						if (tfi->type.length == 1) {
							*(uint8_t*)(htdata.get() + hfi->offset + 4) = *(uint8_t*)(ti->data + tfi->offset);
						} else {
							DPRINTF("Cannot process associated mask with invalid length %d\n", tfi->type.length);
						}
					}
				}
				break;

			default:
				break;
			}
			continue;
		}

		msg(MSG_FATAL, "Flow to be buffered did not contain %s field", typeid2string(hfi->type.id));
		continue;
	}

	/* ...then buffer it */
	bufferDataBlock(htdata);
}

/**
 * Adds a set of callback functions to the list of functions to call when Templates or Records have to be sent
 * @param flowSink the destination module
 */
void Hashtable::addFlowSink(FlowSink* flowSink) {
	FlowSource::addFlowSink(flowSink);

	/* Immediately pass the Hashtable's DataTemplate to the new Callback receiver */
	boost::shared_ptr<IpfixDataTemplateRecord> ipfixRecord(new IpfixDataTemplateRecord);
	ipfixRecord->sourceID.reset();
	ipfixRecord->dataTemplateInfo = dataTemplate;
	flowSink->push(ipfixRecord);
}

std::string Hashtable::getStatistics()
{
	ostringstream oss;
	oss << "Hashtable: number of hashtable entries      : " << statTotalEntries << endl;
	oss << "Hashtable: number of empty hashtable buckets: " << statEmptyBuckets << endl;
	return oss.str();
}
