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

#include "FlowHashtable.h"
#include "modules/ipfix/IpfixRecord.hpp"

#include "common/crc.hpp"
#include "common/Misc.h"
#include "modules/ipfix/IpfixPrinter.hpp"



FlowHashtable::FlowHashtable(Source<IpfixRecord*>* recordsource, Rule* rule,
		uint16_t minBufferTime, uint16_t maxBufferTime, uint8_t hashbits)
	: BaseHashtable(recordsource, rule, minBufferTime, maxBufferTime, hashbits)
{
}


FlowHashtable::~FlowHashtable()
{
	if (revKeyMapper) delete[] revKeyMapper;
	if (switchArray) delete[] switchArray;
}





/**
 * Adds (or otherwise aggregates) @c deltaData to @c baseData
 * @returns 0 if successful, else 1 (when no aggregation mechanism was found)
 */
int FlowHashtable::aggregateField(TemplateInfo::FieldInfo* basefi, TemplateInfo::FieldInfo* deltafi, IpfixRecord::Data* base,
								  IpfixRecord::Data* delta) {
	IpfixRecord::Data* baseData = base+basefi->offset;
	IpfixRecord::Data* deltaData = delta+deltafi->offset;
	InformationElement::IeInfo* type = &basefi->type;
	uint32_t* plen;

	switch (type->enterprise) {
		case 0:
			switch (type->id) {
				case IPFIX_TYPEID_flowStartSysUpTime:
				case IPFIX_TYPEID_flowStartSeconds:
				case IPFIX_TYPEID_flowStartMicroseconds:
					if (type->length != 4) {
						DPRINTF("unsupported length %d for type %d", type->length, type->id);
						return 1;
					}

					*(uint32_t*)baseData = lesserUint32Nbo(*(uint32_t*)baseData, *(uint32_t*)deltaData);
					return 0;

				case IPFIX_TYPEID_flowStartMilliseconds:
				case IPFIX_TYPEID_flowStartNanoseconds:
					if (type->length != 8) {
						DPRINTF("unsupported length %d for type %d", type->length, type->id);
						return 1;
					}

					*(uint64_t*)baseData = lesserUint64Nbo(*(uint64_t*)baseData, *(uint64_t*)deltaData);
					return 0;

				case IPFIX_TYPEID_flowEndSysUpTime:
				case IPFIX_TYPEID_flowEndSeconds:
				case IPFIX_TYPEID_flowEndMicroseconds:
					if (type->length != 4) {
						DPRINTF("unsupported length %d for type %d", type->length, type->id);
						return 1;
					}

					*(uint32_t*)baseData = greaterUint32Nbo(*(uint32_t*)baseData, *(uint32_t*)deltaData);
					return 0;

				case IPFIX_TYPEID_flowEndMilliseconds:
				case IPFIX_TYPEID_flowEndNanoseconds:
					if (type->length != 8) {
						DPRINTF("unsupported length %d for type %d", type->length, type->id);
						return 1;
					}

					*(uint64_t*)baseData = greaterUint64Nbo(*(uint64_t*)baseData, *(uint64_t*)deltaData);
					return 0;

				case IPFIX_TYPEID_octetTotalCount:
				case IPFIX_TYPEID_octetDeltaCount:
				case IPFIX_TYPEID_postOctetDeltaCount:
				case IPFIX_TYPEID_packetTotalCount:
				case IPFIX_TYPEID_packetDeltaCount:
				case IPFIX_TYPEID_postPacketDeltaCount:
				case IPFIX_TYPEID_droppedOctetDeltaCount:
				case IPFIX_TYPEID_droppedPacketDeltaCount:
					if (type->length != 8) {
						DPRINTF("unsupported length %d for type %d", type->length, type->id);
						return 1;
					}
					*(uint64_t*)baseData = addUint64Nbo(*(uint64_t*)baseData, *(uint64_t*)deltaData);
					return 0;

				case IPFIX_TYPEID_tcpControlBits:
					ASSERT(type->length==1, "unsupported length for type");
					*((uint8_t*)baseData) |= *((uint8_t*)deltaData);
					return 0;
			}
			break;

		case IPFIX_PEN_reverse:
			switch (type->id) {
				case IPFIX_TYPEID_flowStartSeconds:
					if (type->length != 4) {
						DPRINTF("unsupported length %d for type %d", type->length, type->id);
						return 1;
					}
					if (*(uint32_t*)baseData == 0)
						*((uint32_t*)baseData) = *((uint32_t*)deltaData);
					else if (*(uint32_t*)deltaData > 0)		// be certain that deltaData is not 0!
						*(uint32_t*)baseData = lesserUint32Nbo(*(uint32_t*)baseData, *(uint32_t*)deltaData);
					return 0;

				case IPFIX_TYPEID_flowStartMilliseconds:
				case IPFIX_TYPEID_flowStartNanoseconds:
					if (type->length != 8) {
						DPRINTF("unsupported length %d for type %d", type->length, type->id);
						return 1;
					}

					if (*(uint64_t*)baseData == 0)
						*((uint64_t*)baseData) = *((uint64_t*)deltaData);
					else if (*(uint64_t*)deltaData > 0)		// be certain that deltaData is not 0!
						*(uint64_t*)baseData = lesserUint64Nbo(*(uint64_t*)baseData, *(uint64_t*)deltaData);
					return 0;

				case IPFIX_TYPEID_flowEndSeconds:
					if (type->length != 4) {
						DPRINTF("unsupported length %d for type %d", type->length, type->id);
						return 1;
					}

					*(uint32_t*)baseData = greaterUint32Nbo(*(uint32_t*)baseData, *(uint32_t*)deltaData);
					return 0;

				case IPFIX_TYPEID_flowEndMilliseconds:
				case IPFIX_TYPEID_flowEndNanoseconds:
					if (type->length != 8) {
						DPRINTF("unsupported length %d for type %d", type->length, type->id);
						return 1;
					}

					*(uint64_t*)baseData = greaterUint64Nbo(*(uint64_t*)baseData, *(uint64_t*)deltaData);
					return 0;

				case IPFIX_TYPEID_octetTotalCount:
				case IPFIX_TYPEID_octetDeltaCount:
				case IPFIX_TYPEID_packetTotalCount:
				case IPFIX_TYPEID_packetDeltaCount:
					*(uint64_t*)baseData = addUint64Nbo(*(uint64_t*)baseData, *(uint64_t*)deltaData);
					return 0;

				case IPFIX_TYPEID_tcpControlBits:
					ASSERT(type->length==1, "unsupported length for type");
					*((uint8_t*)baseData) |= *((uint8_t*)deltaData);
					return 0;

			}
			break;

		case IPFIX_PEN_vermont:
			switch (type->id) {
				case IPFIX_ETYPEID_frontPayload:
					plen = reinterpret_cast<uint32_t*>(base+basefi->privDataOffset+4);
					// only copy payload if it was not inserted into the field yet
					if (*plen == 0) {
						memcpy(baseData, deltaData, type->length);
						*plen = *reinterpret_cast<uint32_t*>(delta+deltafi->privDataOffset+4);
					}
					return 0;

				case IPFIX_ETYPEID_frontPayloadLen:
					// ensure that this field is only written once with data (same behavior as for frontpayload)
					if (*((uint32_t*)baseData)==0) {
						*((uint32_t*)baseData) = *((uint32_t*)deltaData);
					}
					return 0;

				case IPFIX_ETYPEID_maxPacketGap:
					*(uint32_t*)baseData = greaterUint32Nbo(*(uint32_t*)baseData, *(uint32_t*)deltaData);
					// FIXME: additionally, start and end times of both flows should be compared
					// - this one could be the greatest gap!
					return 0;

				case IPFIX_ETYPEID_frontPayloadPktCount:
					*(uint32_t*)baseData = addUint32Nbo(*(uint32_t*)baseData, *(uint32_t*)deltaData);
					return 0;

			}
			break;

		case IPFIX_PEN_reverse|IPFIX_PEN_vermont:
			switch (type->id) {
				case IPFIX_ETYPEID_frontPayload:
					plen = reinterpret_cast<uint32_t*>(base+basefi->privDataOffset+4);
					// only copy payload if it was not inserted into the field yet
					if (*plen == 0) {
						memcpy(baseData, deltaData, type->length);
						*plen = *reinterpret_cast<uint32_t*>(delta+deltafi->privDataOffset+4);
					}
					return 0;

				case IPFIX_ETYPEID_frontPayloadLen:
					// ensure that this field is only written once with data (same behavior as for frontpayload)
					if (*((uint32_t*)baseData)==0) {
						*((uint32_t*)baseData) = *((uint32_t*)deltaData);
					}

				case IPFIX_ETYPEID_maxPacketGap:
					*(uint32_t*)baseData = greaterUint32Nbo(*(uint32_t*)baseData, *(uint32_t*)deltaData);
					// FIXME: additionally, start and end times of both flows should be compared
					// - this one could be the greatest gap!
					return 0;
				return 0;
			}
			break;
	}
	DPRINTF("non-aggregatable type: %s", type->toString().c_str());
	return 1;
}

/**
 * compares two 32bit integers in NBO format
 * @returns 0 if they are equal
 * @returns 1 if second is bigger than first argument
 * @returns -1 if first is bigger than second argument
 */
int FlowHashtable::compare4ByteField(IpfixRecord::Data* baseFlow, TemplateInfo::FieldInfo* baseFi, IpfixRecord::Data* flow, TemplateInfo::FieldInfo* deltaFi)
{
	IpfixRecord::Data* baseData = baseFlow+baseFi->offset;
	IpfixRecord::Data* deltaData = flow+deltaFi->offset;

	uint32_t base = ntohl(*((uint32_t*)baseData));
	uint32_t delta = ntohl(*((uint32_t*)deltaData));
	if (base<delta) return -1;
	else if (base==delta) return 0;
	else return 1;
}

/**
 * compares two 64bit integers in NBO format
 * @returns 0 if they are equal
 * @returns 1 if second is bigger than first argument
 * @returns -1 if first is bigger than second argument
 */
int FlowHashtable::compare8ByteField(IpfixRecord::Data* baseFlow, TemplateInfo::FieldInfo* baseFi, IpfixRecord::Data* flow, TemplateInfo::FieldInfo* deltaFi)
{
	IpfixRecord::Data* baseData = baseFlow+baseFi->offset;
	IpfixRecord::Data* deltaData = flow+deltaFi->offset;

	uint64_t base = ntohll(*((uint64_t*)baseData));
	uint64_t delta = ntohll(*((uint64_t*)deltaData));
	if (base<delta) return -1;
	else if (base==delta) return 0;
	else return 1;
}

/**
 * Adds (or otherwise aggregates) pertinent fields of @c flow to @c baseFlow
 * @returns 1 if flow should be reversed (for biflow aggregation: if a later incoming
 *          flow happened before, so that connection should be reversed
 * @returns 0 if everything was ok
 */
int FlowHashtable::aggregateFlow(IpfixRecord::Data* baseFlow, IpfixRecord::Data* flow, bool reverse)
{
	int i;
	int result = 0;

	for (i = 0; i < dataTemplate->fieldCount; i++) {
		TemplateInfo::FieldInfo* fi = &dataTemplate->fieldInfo[i];

		if(!isToBeAggregated(fi->type)) {
			continue;
		}
		if (reverse && fi->type.enterprise == 0) {
			int secequality = -2;
			int msequality = -2;
			int nsequality = -2;
			// look if current field is to be reverted, if yes, map fields
			// additionally check, if flow should be reversed
			switch (fi->type.id) {
				case IPFIX_TYPEID_flowStartSeconds:
					secequality = compare4ByteField(baseFlow, fi, flow, fi);
					break;
				case IPFIX_TYPEID_flowStartMilliseconds:
					msequality = compare8ByteField(baseFlow, fi, flow, fi);
					break;
				case IPFIX_TYPEID_flowStartNanoseconds:
					nsequality = compare8ByteField(baseFlow, fi, flow, fi);
					break;
			}
			aggregateField(&dataTemplate->fieldInfo[flowReverseMapper[i]], fi, baseFlow, flow);

			// check if flow should be reversed
			if (nsequality>0 || secequality>0 || msequality>0) result = 1;
		} else {
			aggregateField(fi, fi, baseFlow, flow);
		}
	}

	atomic_release(&aggInProgress);

	return result;
}


/**
 * Returns a hash value corresponding to all variable, non-aggregatable fields of a flow
 */
uint32_t FlowHashtable::getHash(IpfixRecord::Data* data, bool reverse) {
	int i;

	uint32_t hash = 0;
	for (i = 0; i < dataTemplate->fieldCount; i++) {
		if(isToBeAggregated(dataTemplate->fieldInfo[i].type)) {
			continue;
		}
		uint32_t idx = (reverse ? revKeyMapper[i] : i);
		hash = crc32(hash,
				dataTemplate->fieldInfo[idx].type.length,
				(char*)data + dataTemplate->fieldInfo[idx].offset);
	}

	return hash & (htableSize-1);
}

/**
 * Checks if two data fields are binary equal
 * @return 1 if fields are equal
 */
int FlowHashtable::equalRaw(InformationElement::IeInfo* data1Type, IpfixRecord::Data* data1, InformationElement::IeInfo* data2Type, IpfixRecord::Data* data2)
{
	int i;

	if(data1Type->id != data2Type->id) return 0;
	if(data1Type->length != data2Type->length) return 0;
	if(data1Type->enterprise != data2Type->enterprise) return 0;

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
int FlowHashtable::equalFlow(IpfixRecord::Data* flow1, IpfixRecord::Data* flow2, bool reverse)
{
	int i;

	if(flow1 == flow2) return 1;

	for(i = 0; i < dataTemplate->fieldCount; i++) {
		TemplateInfo::FieldInfo* fi = &dataTemplate->fieldInfo[i];

		if(isToBeAggregated(fi->type)) {
			continue;
		}

		if (reverse) {
			TemplateInfo::FieldInfo* rfi = &dataTemplate->fieldInfo[revKeyMapper[i]];
			if(!equalRaw(&fi->type, flow1 + fi->offset,
						 &fi->type /* both types *must* be equal, although they may not */,
						 flow2 + rfi->offset)) {
				return 0;
			}
		} else {
			if(!equalRaw(&fi->type, flow1 + fi->offset, &fi->type, flow2 + fi->offset)) {
				return 0;
			}
		}
	}

	return 1;
}


HashtableBucket* FlowHashtable::lookupBucket(uint32_t hash, IpfixRecord::Data* data, bool reverse, HashtableBucket** prevBucket)
{
	HashtableBucket* bucket = buckets[hash];
	*prevBucket = NULL;

	if (bucket != NULL) {
		/* This slot is already used, search spill chain for equal flow */
		while (bucket != NULL) {
			if (equalFlow(bucket->data.get(), data, reverse)) {
				return bucket;
			}

			*prevBucket = bucket;
			bucket = (HashtableBucket*)bucket->next;
		}
	}

	return NULL;
}



/**
 * Inserts a data block into the hashtable
 */
void FlowHashtable::bufferDataBlock(boost::shared_array<IpfixRecord::Data> data, uint32_t flowStartTimeSeconds, uint32_t flowEndTimeSeconds)
{
	statRecordsReceived++;

	// adapt the current time based on the flowStartTimeSeconds
	// we are conservative and use the flowStartTimeSeconds in order
	// to not make the aggregation thread remove flows prematurely
	if (flowStartTimeSeconds > now) {
		now = flowStartTimeSeconds;
	}
	uint32_t nhash = getHash(data.get(), false);
	DPRINTFL(MSG_VDEBUG, "nhash=%u", nhash);
	HashtableBucket* prevbucket;
	HashtableBucket* bucket = lookupBucket(nhash, data.get(), false, &prevbucket);

	bool flowfound = false;
	bool expiryforced = false;

	if (bucket != NULL) {
		DPRINTFL(MSG_VDEBUG, "aggregating flow");
		// check if we need to expire the flow. we use a simple 
		// distribution scheme to distribute flow counters among 
		// flows that overlap with the active timeouts: 
		// assign all counters to the first flow. Hence we do 
		// not need to check whether the flow overlaps (i.e
		// flowStartTimeSeconds < bucket->expireTime and flowEndSeconds > bucket->expireTime)
		if (flowStartTimeSeconds > bucket->expireTime || flowStartTimeSeconds > bucket->forceExpireTime){
			expiryforced = true;
			bucket->forceExpiry = true;
			removeBucket(bucket);
		} else {
			flowfound = true;
			aggregateFlow(bucket->data.get(), data.get(), false);
			bucket->expireTime = flowEndTimeSeconds + minBufferTime;
			if (bucket->forceExpireTime>bucket->expireTime) {
				exportList.remove(bucket->listNode);
				exportList.push(bucket->listNode);
				removeBucket(bucket);
			}
		}
	}
	if (biflowAggregation && !flowfound && !expiryforced) {
		// try reverse flow
		uint32_t rhash = getHash(data.get(), true);
		DPRINTFL(MSG_VDEBUG, "rhash=%u", rhash);
		bucket = lookupBucket(rhash, data.get(), true, &prevbucket);
		if (bucket != NULL) {
			if (flowStartTimeSeconds > bucket->expireTime || flowStartTimeSeconds > bucket->forceExpireTime) {
				bucket->forceExpiry = true;
				expiryforced = true;
				removeBucket(bucket);
			} else {
				flowfound = true;
				DPRINTFL(MSG_VDEBUG, "aggregating reverse flow");
				int must_reverse = aggregateFlow(bucket->data.get(), data.get(), true);
				if (must_reverse == 1) {
					DPRINTFL(MSG_VDEBUG, "reversing whole flow");
					// reverse flow
					//msg(MSG_ERROR, "Reversing flow");
					reverseFlowBucket(bucket);
					// delete reference from hash table
					if (prevbucket==NULL)
						buckets[rhash] = bucket->next;
					else {
						prevbucket->next = bucket->next;
					}
					if (bucket->next!=NULL)
						bucket->next->prev = prevbucket;
					// insert into hash table again
					nhash = getHash(bucket->data.get(), false);
					DPRINTFL(MSG_VDEBUG, "nhash=%u", nhash);
					bucket->next = buckets[nhash];
					bucket->hash = nhash;
					buckets[nhash] = bucket;
					bucket->prev = 0;
					if (bucket->next != NULL) bucket->next->prev = bucket;
					bucket->expireTime = flowEndTimeSeconds + minBufferTime;
					if (bucket->forceExpireTime>bucket->expireTime) {
						exportList.remove(bucket->listNode);
						exportList.push(bucket->listNode);
						removeBucket(bucket);
					}
				}
			}
		}
	}
	if (!flowfound || expiryforced) {
		DPRINTFL(MSG_VDEBUG, "creating new bucket");
		HashtableBucket* n = buckets[nhash];
		buckets[nhash] = createBucket(data, 0, n, 0, nhash, flowStartTimeSeconds); // FIXME: insert observationDomainID!
		buckets[nhash]->inTable = true;
		if (n != NULL) n->prev = buckets[nhash];
		BucketListElement* node = hbucketIM.getNewInstance();
		node->reset();
		buckets[nhash]->listNode = node;
		node->bucket = buckets[nhash];
		exportList.push(node);
	}
	atomic_release(&aggInProgress);
}

/**
 * Copies \c srcData to \c dstData applying \c modifier.
 * Takes care to pad \c srcData with zero-bytes in case it is shorter than \c dstData.
 */
void FlowHashtable::copyData(TemplateInfo::FieldInfo* dstFI, IpfixRecord::Data* dst, TemplateInfo::FieldInfo* srcFI, IpfixRecord::Data* src, Rule::Field::Modifier modifier)
{
	InformationElement::IeInfo* dstType = &dstFI->type;
	InformationElement::IeInfo* srcType = &srcFI->type;
	IpfixRecord::Data* dstData = dst+dstFI->offset;
	IpfixRecord::Data* srcData = src+srcFI->offset;
	if((dstType->id != srcType->id) || (dstType->enterprise != srcType->enterprise)) {
		DPRINTF("copyData: Tried to copy field to destination of different type\n");
		return;
	}

	/* Copy data, care for length differences */
	uint32_t copylen = 0;
	if(dstType->length == srcType->length) {
		copylen = srcType->length;
		memcpy(dstData, srcData, copylen);

	} else if(dstType->length > srcType->length) {
		copylen = srcType->length;
		/* TODO: We simply pad with zeroes - will this always be correct? */
		if (dstType->enterprise==0) {
			switch (dstType->id) {
				/* Fields of type IPv4Address-type and payload are padded on the right */
				case IPFIX_TYPEID_sourceIPv4Address:
				case IPFIX_TYPEID_destinationIPv4Address:
					bzero(dstData+copylen, dstType->length-copylen);
					memcpy(dstData, srcData, copylen);
					break;

				default:
					bzero(dstData, dstType->length-srcType->length);
					memcpy(dstData + dstType->length - srcType->length, srcData, copylen);
					break;
			}
		} else if ((dstType->enterprise&IPFIX_PEN_vermont) && dstType->id==IPFIX_ETYPEID_frontPayload) {
			bzero(dstData+copylen, dstType->length-copylen);
			memcpy(dstData, srcData, copylen);
		} else {
			bzero(dstData, dstType->length-srcType->length);
			memcpy(dstData + dstType->length - srcType->length, srcData, copylen);
		}

	} else {
		if ((dstType->enterprise&IPFIX_PEN_vermont) && dstType->id == IPFIX_ETYPEID_frontPayload) {
			copylen = dstType->length;
			memcpy(dstData, srcData, copylen);
		} else {
			DPRINTF("Target buffer too small. Buffer expected %s of length %d, got one with length %dn", srcType->toString().c_str(), srcType->length, dstType->length);
			return;
		}
	}

	// save used length of payload field
	if ((dstType->enterprise&IPFIX_PEN_vermont) && dstType->id == IPFIX_ETYPEID_frontPayload) {
		*reinterpret_cast<uint32_t*>(&dst[dstFI->privDataOffset+4]) = copylen;
	}

	/* Apply modifier */
	if(modifier == Rule::Field::DISCARD) {
		DPRINTF("Tried to copy data w/ having field modifier set to discard\n");
		return;
	} else if((modifier == Rule::Field::KEEP) || (modifier == Rule::Field::AGGREGATE)) {

	} else if((modifier >= Rule::Field::MASK_START) && (modifier <= Rule::Field::MASK_END)) {

		if((dstType->id != IPFIX_TYPEID_sourceIPv4Address) && (dstType->id != IPFIX_TYPEID_destinationIPv4Address)) {
			DPRINTF("Tried to apply mask to %s field\n", dstType->toString().c_str());
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
 * Buffer passed flow (containing fixed-value fields) in Hashtable @c ht
 */
void FlowHashtable::aggregateDataRecord(IpfixDataRecord* record)
{
	DPRINTF("called");

	boost::shared_ptr<TemplateInfo> ti = record->templateInfo;
	IpfixRecord::Data* data = record->data;

	// the following lock should almost never fail (only during reconfiguration)
	while (atomic_lock(&aggInProgress)) {
		timespec req;
		req.tv_sec = 0;
		req.tv_nsec = 50000000;
		nanosleep(&req, &req);
	}

	int i;
	uint32_t startSec = 0;
	uint32_t endSec = 0;

	/* Create data block to be inserted into buffer... */
	boost::shared_array<IpfixRecord::Data> htdata(new IpfixRecord::Data[fieldLength+privDataLength]);

	// set private data fields to zero
	memset(htdata.get()+fieldLength, 0, privDataLength);

	for (i = 0; i < dataTemplate->fieldCount; i++) {
		TemplateInfo::FieldInfo* hfi = &dataTemplate->fieldInfo[i];

		bool fieldFilled = false;

		/* Copy from matching variable field, should it exist */
		TemplateInfo::FieldInfo* tfi = ti->getFieldInfo(hfi->type);
		if (tfi) {
			// this path is normal for normal flow data records!
			fieldFilled = true;
			copyData(hfi, htdata.get(), tfi, data, fieldModifier[i]);

			// obtain the start time of the flow in seconds
			switch(tfi->type.id) {
				case IPFIX_TYPEID_flowStartSeconds:
					if(hfi->type.length != 4) {
						DPRINTF("Cannot process flowStartSeconds with invalid length %d", hfi->type.length);
					}
					startSec = htonl(*(uint32_t*)(data + tfi->offset));
					break;
				case IPFIX_TYPEID_flowStartMilliseconds:
					if(hfi->type.length != 8) {
						DPRINTF("Cannot process flowStartMilliseconds with invalid length %d", hfi->type.length);
					}
					startSec = htonll(*(uint64_t*)(data + tfi->offset)) / 1000;
					break;
				case IPFIX_TYPEID_flowStartMicroseconds:
					if(hfi->type.length != 8) {
						DPRINTF("Cannot process flowStartMicroseconds with invalid length %d", hfi->type.length);
					}
					startSec = htonll(*(uint64_t*)(data + tfi->offset)) / 1000000;
					break;
				case IPFIX_TYPEID_flowStartNanoseconds:
					if(hfi->type.length != 8) {
						DPRINTF("Cannot process flowStartNanoseconds with invalid length %d", hfi->type.length);
					}
					startSec = htonll(*(uint64_t*)(data + tfi->offset)) / 1000000000;
					break;
				case IPFIX_TYPEID_flowEndSeconds:
					if(hfi->type.length != 4) {
						DPRINTF("Cannot process flowEndSeconds with invalid length %d", hfi->type.length);
					}
					endSec = htonl(*(uint32_t*)(data + tfi->offset));
					break;
				case IPFIX_TYPEID_flowEndMilliseconds:
					if(hfi->type.length != 8) {
						DPRINTF("Cannot process flowEndMilliseconds with invalid length %d", hfi->type.length);
					}
					endSec = htonll(*(uint64_t*)(data + tfi->offset)) / 1000;
					break;
				case IPFIX_TYPEID_flowEndMicroseconds:
					if(hfi->type.length != 8) {
						DPRINTF("Cannot process flowEndMicroseconds with invalid length %d", hfi->type.length);
					}
					endSec = htonll(*(uint64_t*)(data + tfi->offset)) / 1000000;
					break;
				case IPFIX_TYPEID_flowEndNanoseconds:
					if(hfi->type.length != 8) {
						DPRINTF("Cannot process flowEndNanoseconds with invalid length %d", hfi->type.length);
					}
					endSec = htonll(*(uint64_t*)(data + tfi->offset)) / 1000000000;
					break;


				default:
					break;

			}

			/* copy associated mask, should there be one */
			switch (hfi->type.id) {

				case IPFIX_TYPEID_sourceIPv4Address:
					tfi = ti->getFieldInfo(IPFIX_TYPEID_sourceIPv4PrefixLength, 0);
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
					tfi = ti->getFieldInfo(IPFIX_TYPEID_destinationIPv4PrefixLength, 0);
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

		// If this is a Data Record, search in fixed value fields
		if((ti->setId==TemplateInfo::IpfixDataTemplate) && (ti->dataCount > 0)) {
			/* No matching variable field. Copy from matching fixed field, should it exist */
			tfi = ti->getDataInfo(hfi->type);
			if (tfi) {
				fieldFilled = true;
				copyData(hfi, htdata.get(), tfi, ti->data, fieldModifier[i]);

				/* copy associated mask, should there be one */
				switch (hfi->type.id) {

					case IPFIX_TYPEID_sourceIPv4Address:
						tfi = ti->getDataInfo(IPFIX_TYPEID_sourceIPv4PrefixLength, 0);
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
						tfi = ti->getDataInfo(IPFIX_TYPEID_destinationIPv4PrefixLength, 0);
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
		}

		if (!fieldFilled) {
			DPRINTF("Flow to be buffered did not contain %s field\n", hfi->type.toString().c_str());
			// if field was not copied, fill it with 0
			memset(htdata.get() + hfi->offset, 0, hfi->type.length);
		}

		continue;
	}

	/* ...then buffer it */
	bufferDataBlock(htdata, startSec, endSec);

	atomic_release(&aggInProgress);
}
