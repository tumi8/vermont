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

#include <string.h>
#include <netinet/in.h>
#include <time.h>

#include "Hashtable.hpp"
#include "crc16.hpp"
#include "ipfix.hpp"

#include "ipfixlolib/ipfixlolib.h"

#include "msg.h"

/**
 * Initializes memory for a new bucket in @c ht containing @c data
 */
Hashtable::Bucket* Hashtable::createBucket(IpfixRecord::Data* data) {
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
	for (FlowSinks::iterator i = flowSinks.begin(); i != flowSinks.end(); i++) {
		(*i)->onDataDataRecord(0, dataTemplate, fieldLength, bucket->data);
	}

	recordsSent++;
}

/**
 * De-allocates memory used by the given @c bucket
 */
void Hashtable::destroyBucket(Hashtable::Bucket* bucket) {
	free(bucket->data); //TODO: is this correct?
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

	dataTemplate = (IpfixRecord::DataTemplateInfo*)malloc(sizeof(IpfixRecord::DataTemplateInfo));
	dataTemplate->id=rule->id;
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
	for (FlowSinks::iterator i = flowSinks.begin(); i != flowSinks.end(); i++) {
		(*i)->onDataTemplateDestruction(0, dataTemplate);
	}
	*/

	free(dataTemplate->fieldInfo);
	free(fieldModifier);
	free(dataTemplate->dataInfo);
	free(dataTemplate->data);
	free(dataTemplate);
}

/**
 * Exports all expired flows and removes them from the buffer
 */
void Hashtable::expireFlows() {
	uint32_t now = time(0);
	int i;

	/* check each hash bucket's spill chain */
	for (i = 0; i < bucketCount; i++) if (buckets[i] != 0) {
		Hashtable::Bucket* bucket = buckets[i];
		Hashtable::Bucket* pred = 0;

		/* iterate over spill chain */
		while (bucket != 0) {
			Hashtable::Bucket* nextBucket = (Hashtable::Bucket*)bucket->next;
			if ((now > bucket->expireTime) || (now > bucket->forceExpireTime)) {
				if(now > bucket->expireTime) DPRINTF("expireFlows: normal expiry\n");
				if(now > bucket->forceExpireTime) DPRINTF("expireFlows: forced expiry\n");

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
	}
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
	case IPFIX_TYPEID_flowStartMilliSeconds:
	case IPFIX_TYPEID_flowStartMicroSeconds:
	case IPFIX_TYPEID_flowStartNanoSeconds:
		if (type->length != 4) {
			DPRINTF("aggregateField: unsupported length: %d", type->length);
                        goto out;
		}

		*(uint32_t*)baseData = lesserUint32Nbo(*(uint32_t*)baseData, *(uint32_t*)deltaData);
		break;

	case IPFIX_TYPEID_flowEndSysUpTime:
	case IPFIX_TYPEID_flowEndSeconds:
	case IPFIX_TYPEID_flowEndMilliSeconds:
	case IPFIX_TYPEID_flowEndMicroSeconds:
	case IPFIX_TYPEID_flowEndNanoSeconds:
		if (type->length != 4) {
			DPRINTF("aggregateField: unsupported length: %d", type->length);
			goto out;
		}

		*(uint32_t*)baseData = greaterUint32Nbo(*(uint32_t*)baseData, *(uint32_t*)deltaData);
		break;

	case IPFIX_TYPEID_octetDeltaCount:
	case IPFIX_TYPEID_postOctetDeltaCount:
	case IPFIX_TYPEID_packetDeltaCount:
	case IPFIX_TYPEID_postPacketDeltaCount:
	case IPFIX_TYPEID_droppedOctetDeltaCount:
	case IPFIX_TYPEID_droppedPacketDeltaCount:
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
			DPRINTF("aggregateField: unsupported length: %d", type->length);
			goto out;
		}
		break;

	default:
		DPRINTF("aggregateField: non-aggregatable type: %d", type->id);
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
void Hashtable::bufferDataBlock(IpfixRecord::Data* data)
{
	recordsReceived++;

	uint16_t hash = getHash(data);
	Hashtable::Bucket* bucket = buckets[hash];

	if (bucket == 0) {
		/* This slot is still free, place the bucket here */
		DPRINTF("bufferDataBlock: creating bucket\n");
		buckets[hash] = createBucket(data);
		return;
	}

	/* This slot is already used, search spill chain for equal flow */
	while(1) {
		if (equalFlow(bucket->data, data)) {
			DPRINTF("appending to bucket\n");

			aggregateFlow(bucket->data, data);
			bucket->expireTime = time(0) + minBufferTime;

			/* The flow's data block is no longer needed */
			free(data);
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
	int i;

	/* Create data block to be inserted into buffer... */
	IpfixRecord::Data* htdata = (IpfixRecord::Data*)malloc(fieldLength);

	for (i = 0; i < dataTemplate->fieldCount; i++) {
		IpfixRecord::FieldInfo* hfi = &dataTemplate->fieldInfo[i];
		IpfixRecord::FieldInfo* tfi = getTemplateFieldInfo(ti, &hfi->type);

		if(!tfi) {
			DPRINTF("Flow to be buffered did not contain %s field\n", typeid2string(hfi->type.id));
			continue;
		}

		copyData(&hfi->type, htdata + hfi->offset, &tfi->type, data + tfi->offset, fieldModifier[i]);

		/* copy associated mask, should there be one */
		switch (hfi->type.id) {
		case IPFIX_TYPEID_sourceIPv4Address:
			tfi = getTemplateFieldInfo(ti, IPFIX_TYPEID_sourceIPv4Mask, 0);

			if(tfi) {
				if(hfi->type.length != 5) {
					DPRINTF("Tried to set mask of length %d IP address\n", hfi->type.length);
				} else {
					if(tfi->type.length == 1) {
						*(uint8_t*)(htdata + hfi->offset + 4) = *(uint8_t*)(data + tfi->offset);
					} else {
						DPRINTF("Cannot process associated mask with invalid length %d\n", tfi->type.length);
					}
				}
			}
			break;

		case IPFIX_TYPEID_destinationIPv4Address:
			tfi = getTemplateFieldInfo(ti, IPFIX_TYPEID_destinationIPv4Mask, 0);

			if(tfi) {
				if(hfi->type.length != 5) {
					DPRINTF("Tried to set mask of length %d IP address\n", hfi->type.length);
				} else {
					if(tfi->type.length == 1) {
						*(uint8_t*)(htdata + hfi->offset + 4) = *(uint8_t*)(data + tfi->offset);
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
 * Buffer passed flow (containing fixed-value fields) in Hashtable @c ht
 */
void Hashtable::aggregateDataTemplateData(IpfixRecord::DataTemplateInfo* ti, IpfixRecord::Data* data)
{
	int i;

	/* Create data block to be inserted into buffer... */
	IpfixRecord::Data* htdata = (IpfixRecord::Data*)malloc(fieldLength);

	for (i = 0; i < dataTemplate->fieldCount; i++) {
		IpfixRecord::FieldInfo* hfi = &dataTemplate->fieldInfo[i];

		/* Copy from matching variable field, should it exist */
		IpfixRecord::FieldInfo* tfi = getDataTemplateFieldInfo(ti, &hfi->type);
		if(tfi) {
			copyData(&hfi->type, htdata + hfi->offset, &tfi->type, data + tfi->offset, fieldModifier[i]);

			/* copy associated mask, should there be one */
			switch (hfi->type.id) {

			case IPFIX_TYPEID_sourceIPv4Address:
			tfi = getDataTemplateFieldInfo(ti, IPFIX_TYPEID_sourceIPv4Mask, 0);
				if(tfi) {
					if(hfi->type.length != 5) {
						DPRINTF("Tried to set mask of length %d IP address\n", hfi->type.length);
					} else {
						if(tfi->type.length == 1) {
							*(uint8_t*)(htdata + hfi->offset + 4) = *(uint8_t*)(data + tfi->offset);
						} else {
							DPRINTF("Cannot process associated mask with invalid length %d\n", tfi->type.length);
						}
					}
				}
				break;

			case IPFIX_TYPEID_destinationIPv4Address:
				tfi = getDataTemplateFieldInfo(ti, IPFIX_TYPEID_destinationIPv4Mask, 0);
				if(tfi) {
					if(hfi->type.length != 5) {
						DPRINTF("Tried to set mask of length %d IP address", hfi->type.length);
					} else {
						if(tfi->type.length == 1) {
							*(uint8_t*)(htdata + hfi->offset + 4) = *(uint8_t*)(data + tfi->offset);
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
		tfi = getDataTemplateDataInfo(ti, &hfi->type);
		if(tfi) {
			copyData(&hfi->type, htdata + hfi->offset, &tfi->type, ti->data + tfi->offset, fieldModifier[i]);

			/* copy associated mask, should there be one */
			switch (hfi->type.id) {

			case IPFIX_TYPEID_sourceIPv4Address:
				tfi = getDataTemplateDataInfo(ti, IPFIX_TYPEID_sourceIPv4Mask, 0);
				if(tfi) {
					if(hfi->type.length != 5) {
						DPRINTF("Tried to set mask of length %d IP address\n", hfi->type.length);
					} else {
						if(tfi->type.length == 1) {
							*(uint8_t*)(htdata + hfi->offset + 4) = *(uint8_t*)(ti->data + tfi->offset);
						} else {
							DPRINTF("Cannot process associated mask with invalid length %d\n", tfi->type.length);
						}
					}
				}
				break;

			case IPFIX_TYPEID_destinationIPv4Address:
				tfi = getDataTemplateDataInfo(ti, IPFIX_TYPEID_destinationIPv4Mask, 0);
				if(tfi) {
					if(hfi->type.length != 5) {
						DPRINTF("Tried to set mask of length %d IP address\n", hfi->type.length);
					} else {
						if (tfi->type.length == 1) {
							*(uint8_t*)(htdata + hfi->offset + 4) = *(uint8_t*)(ti->data + tfi->offset);
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

		msg(MSG_FATAL, "AggregateDataTemplateData: Flow to be buffered did not contain %s field", typeid2string(hfi->type.id));
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
	flowSinks.push_back(flowSink);

	/* Immediately pass the Hashtable's DataTemplate to the new Callback receiver */
	flowSink->onDataTemplate(0, dataTemplate);
}

