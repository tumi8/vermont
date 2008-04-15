#include "FlowHashtable.h"
#include "IpfixRecord.hpp"
#include "crc.hpp"

#include "common/Misc.h"



FlowHashtable::FlowHashtable(Source<IpfixRecord*>* recordsource, Rule* rule, 
		uint16_t minBufferTime, uint16_t maxBufferTime)
	: BaseHashtable(recordsource, rule, minBufferTime, maxBufferTime),
		revDataTemplateMapper(NULL),
		biflowAggregation(rule->biflowAggregation)
{ 
	if (biflowAggregation) {
		genBiflowStructs();
	}
}

FlowHashtable::~FlowHashtable()
{
	if (revDataTemplateMapper) delete[] revDataTemplateMapper;
}


void FlowHashtable::genBiflowStructs()
{
	int32_t srcIPIdx = -1;
	int32_t dstIPIdx = -1;
	int32_t srcPortIdx = -1;
	int32_t dstPortIdx = -1;
	
	// search for offsets in dataTemplate
	revDataTemplateMapper = new uint32_t[dataTemplate->fieldCount];
	for (int i=0; i<dataTemplate->fieldCount; i++) {
		IpfixRecord::FieldInfo* fi = &dataTemplate->fieldInfo[i];
		switch (fi->type.id) {
			case IPFIX_TYPEID_sourceIPv4Address:
				srcIPIdx = i;
				break;
			case IPFIX_TYPEID_destinationIPv4Address:
				dstIPIdx = i;
				break;
			case IPFIX_TYPEID_sourceTransportPort:
				srcPortIdx = i;
				break;
			case IPFIX_TYPEID_destinationTransportPort:
				dstPortIdx = i;
				break;
			case IPFIX_ETYPEID_revFlowStartMilliSeconds:
			case IPFIX_ETYPEID_revFlowStartSeconds:
			case IPFIX_ETYPEID_revFlowEndMilliSeconds:
			case IPFIX_ETYPEID_revOctetDeltaCount:
			case IPFIX_ETYPEID_revPacketDeltaCount:
			case IPFIX_ETYPEID_revTcpControlBits:
			case IPFIX_ETYPEID_revFlowEndSeconds:
				mapRevAggIndizes[fi->type.id] = i;
				break;
		}
	}
	
	// check if it's possible to obtain the reverse flow
	if ((srcIPIdx<0) || (dstIPIdx<0) || (srcPortIdx<0) || (dstPortIdx<0)) {
		THROWEXCEPTION("no biflow aggregation possible for current template, but was activated in configuration!");
	}
	
	for (int i=0; i<dataTemplate->fieldCount; i++) {
		IpfixRecord::FieldInfo* fi = &dataTemplate->fieldInfo[i];
		switch (fi->type.id) {
			case IPFIX_TYPEID_sourceIPv4Address:
				revDataTemplateMapper[i] = dstIPIdx;
				break;
			case IPFIX_TYPEID_destinationIPv4Address:
				revDataTemplateMapper[i] = srcIPIdx;
				break;
			case IPFIX_TYPEID_sourceTransportPort:
				revDataTemplateMapper[i] = dstPortIdx;
				break;
			case IPFIX_TYPEID_destinationTransportPort:
				revDataTemplateMapper[i] = srcPortIdx;
				break;
			default:
				revDataTemplateMapper[i] = i;
				break;
		}
	}
	
	
}


/**
 * Adds (or otherwise aggregates) @c deltaData to @c baseData
 */
int FlowHashtable::aggregateField(IpfixRecord::FieldInfo::Type* type, IpfixRecord::Data* baseData, 
								  IpfixRecord::Data* deltaData) {
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
			
		case IPFIX_ETYPEID_revFlowStartSeconds:
			if (type->length != 4) {
				DPRINTF("unsupported length %d for type %d", type->length, type->id);
				goto out;
			}
			if (*(uint32_t*)baseData == 0)
				*((uint32_t*)baseData) = *((uint32_t*)deltaData);
			else if (*(uint32_t*)deltaData > 0)		// be certain that deltaData is not 0!
				*(uint32_t*)baseData = lesserUint32Nbo(*(uint32_t*)baseData, *(uint32_t*)deltaData);
			break;

		case IPFIX_TYPEID_flowStartMilliSeconds:
			if (type->length != 8) {
				DPRINTF("unsupported length %d for type %d", type->length, type->id);
				goto out;
			}

			*(uint64_t*)baseData = lesserUint64Nbo(*(uint64_t*)baseData, *(uint64_t*)deltaData);
			break;
			
		case IPFIX_ETYPEID_revFlowStartMilliSeconds:
			if (type->length != 8) {
				DPRINTF("unsupported length %d for type %d", type->length, type->id);
				goto out;
			}

			if (*(uint64_t*)baseData == 0)
				*((uint64_t*)baseData) = *((uint64_t*)deltaData);
			else if (*(uint64_t*)deltaData > 0)		// be certain that deltaData is not 0!
				*(uint64_t*)baseData = lesserUint64Nbo(*(uint64_t*)baseData, *(uint64_t*)deltaData);
			break;

		case IPFIX_TYPEID_flowEndSysUpTime:
		case IPFIX_TYPEID_flowEndSeconds:
		case IPFIX_TYPEID_flowEndMicroSeconds:
		case IPFIX_TYPEID_flowEndNanoSeconds:
		case IPFIX_ETYPEID_revFlowEndSeconds:
			if (type->length != 4) {
				DPRINTF("unsupported length %d for type %d", type->length, type->id);
				goto out;
			}

			*(uint32_t*)baseData = greaterUint32Nbo(*(uint32_t*)baseData, *(uint32_t*)deltaData);
			break;

		case IPFIX_TYPEID_flowEndMilliSeconds:
		case IPFIX_ETYPEID_revFlowEndMilliSeconds:
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
		case IPFIX_ETYPEID_revOctetDeltaCount:
		case IPFIX_ETYPEID_revPacketDeltaCount:
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
			
		case IPFIX_TYPEID_tcpControlBits:
		case IPFIX_ETYPEID_revTcpControlBits:
			ASSERT(type->length==1, "unsupported length for type");
			*((uint8_t*)baseData) |= *((uint8_t*)deltaData);
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
int FlowHashtable::aggregateFlow(IpfixRecord::Data* baseFlow, IpfixRecord::Data* flow, bool reverse)
{
	int i;

	if(!baseFlow) {
		return 1;
	}

	if(!flow){
		return 1;
	}

	for (i = 0; i < dataTemplate->fieldCount; i++) {
		IpfixRecord::FieldInfo* fi = &dataTemplate->fieldInfo[i];

		if(!isToBeAggregated(fi->type)) {
			continue;
		}
		if (reverse) {
			uint32_t idx = i;
			// look if current field is to be reverted
			map<uint32_t, uint32_t>::iterator iter;
			switch (fi->type.id) {
				case IPFIX_TYPEID_flowStartSeconds:
					iter = mapRevAggIndizes.find(IPFIX_ETYPEID_revFlowStartSeconds);
					if (iter != mapRevAggIndizes.end()) idx = iter->second;
					break;
				case IPFIX_TYPEID_flowStartMilliSeconds:
					iter = mapRevAggIndizes.find(IPFIX_ETYPEID_revFlowStartMilliSeconds);
					if (iter != mapRevAggIndizes.end()) idx = iter->second;
					break;
				case IPFIX_TYPEID_flowEndSeconds:
					iter = mapRevAggIndizes.find(IPFIX_ETYPEID_revFlowEndSeconds);
					if (iter != mapRevAggIndizes.end()) idx = iter->second;
					break;
				case IPFIX_TYPEID_flowEndMilliSeconds:
					iter = mapRevAggIndizes.find(IPFIX_ETYPEID_revFlowEndMilliSeconds);
					if (iter != mapRevAggIndizes.end()) idx = iter->second;
					break;
				case IPFIX_TYPEID_octetDeltaCount:
					iter = mapRevAggIndizes.find(IPFIX_ETYPEID_revOctetDeltaCount);
					if (iter != mapRevAggIndizes.end()) idx = iter->second;
					break;
				case IPFIX_TYPEID_packetDeltaCount:
					iter = mapRevAggIndizes.find(IPFIX_ETYPEID_revPacketDeltaCount);
					if (iter != mapRevAggIndizes.end()) idx = iter->second;
					break;
				case IPFIX_TYPEID_tcpControlBits:
					iter = mapRevAggIndizes.find(IPFIX_ETYPEID_revTcpControlBits);
					if (iter != mapRevAggIndizes.end()) idx = iter->second;
					break;
			}
			aggregateField(&dataTemplate->fieldInfo[idx].type, baseFlow + dataTemplate->fieldInfo[idx].offset, flow + fi->offset);
			
		} else {
			aggregateField(&fi->type, baseFlow + fi->offset, flow + fi->offset);
		}
	}

	atomic_release(&aggInProgress);

	return 0;
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
		uint32_t idx = (reverse ? revDataTemplateMapper[i] : i);
		hash = crc32(hash,
				dataTemplate->fieldInfo[idx].type.length,
				(char*)data + dataTemplate->fieldInfo[idx].offset);
	}

	return hash & (HTABLE_SIZE-1);
}

/**
 * Checks if two data fields are binary equal
 * @return 1 if fields are equal
 */
int FlowHashtable::equalRaw(IpfixRecord::FieldInfo::Type* data1Type, IpfixRecord::Data* data1, IpfixRecord::FieldInfo::Type* data2Type, IpfixRecord::Data* data2) 
{
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
int FlowHashtable::equalFlow(IpfixRecord::Data* flow1, IpfixRecord::Data* flow2, bool reverse)
{
	int i;

	if(flow1 == flow2) return 1;

	for(i = 0; i < dataTemplate->fieldCount; i++) {
		IpfixRecord::FieldInfo* fi = &dataTemplate->fieldInfo[i];

		if(isToBeAggregated(fi->type)) {
			continue;
		}

		if (reverse) {
			IpfixRecord::FieldInfo* rfi = &dataTemplate->fieldInfo[revDataTemplateMapper[i]];
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


BaseHashtable::Bucket* FlowHashtable::lookupBucket(uint32_t hash, IpfixRecord::Data* data, bool reverse)
{
	Bucket* bucket = buckets[hash];
	
	if (bucket != NULL) {
		/* This slot is already used, search spill chain for equal flow */
		while (bucket != NULL) {
			if (equalFlow(bucket->data.get(), data, reverse)) {
				return bucket;
			}
	
			bucket = (Bucket*)bucket->next;
		}
	}
	
	return NULL;
}


/**
 * Inserts a data block into the hashtable
 */
void FlowHashtable::bufferDataBlock(boost::shared_array<IpfixRecord::Data> data)
{
	statRecordsReceived++;

	uint32_t nhash = getHash(data.get(), false);
	Bucket* bucket = lookupBucket(nhash, data.get(), false);
	
	if (bucket != NULL) {
		aggregateFlow(bucket->data.get(), data.get(), false);
		bucket->expireTime = time(0) + minBufferTime;
	} else {
		if (biflowAggregation) {
			// try reverse flow
			uint32_t rhash = getHash(data.get(), true);
			bucket = lookupBucket(rhash, data.get(), true);
			if (bucket != NULL) {
				aggregateFlow(bucket->data.get(), data.get(), true);
				bucket->expireTime = time(0) + minBufferTime;
			}
		}
		if (bucket == NULL) {
			Bucket* n = buckets[nhash];
			buckets[nhash] = createBucket(data, 0); // FIXME: insert observationDomainID!
			buckets[nhash]->next = n;
		}
	}
}

/**
 * Copies \c srcData to \c dstData applying \c modifier.
 * Takes care to pad \c srcData with zero-bytes in case it is shorter than \c dstData.
 */
void FlowHashtable::copyData(IpfixRecord::FieldInfo::Type* dstType, IpfixRecord::Data* dstData, IpfixRecord::FieldInfo::Type* srcType, IpfixRecord::Data* srcData, Rule::Field::Modifier modifier)
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


/**
 * Buffer passed flow in Hashtable @c ht
 */
void FlowHashtable::aggregateTemplateData(IpfixRecord::TemplateInfo* ti, IpfixRecord::Data* data)
{
	DPRINTF("called");

	// the following lock should almost never fail (only during reconfiguration)
	while (atomic_lock(&aggInProgress)) {
		timespec req;
		req.tv_sec = 0;
		req.tv_nsec = 50000000;
		nanosleep(&req, &req);
	}
	int i;

	/* Create data block to be inserted into buffer... */
	boost::shared_array<IpfixRecord::Data> htdata(new IpfixRecord::Data[fieldLength+privDataLength]);

	for (i = 0; i < dataTemplate->fieldCount; i++) {
		IpfixRecord::FieldInfo* hfi = &dataTemplate->fieldInfo[i];
		IpfixRecord::FieldInfo* tfi = ti->getFieldInfo(&hfi->type);

		if(!tfi) {
			DPRINTF("Flow to be buffered did not contain %s field\n", typeid2string(hfi->type.id));
			
			// if field was not copied, fill it with 0
			memset(htdata.get() + hfi->offset, 0, hfi->type.length);
			
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
	atomic_release(&aggInProgress);
}


/**
 * Buffer passed flow (containing fixed-value fields) in Hashtable @c ht
 */
void FlowHashtable::aggregateDataTemplateData(IpfixRecord::DataTemplateInfo* ti, IpfixRecord::Data* data)
{
	DPRINTF("called");
	
	// the following lock should almost never fail (only during reconfiguration)
	while (atomic_lock(&aggInProgress)) {
		timespec req;
		req.tv_sec = 0;
		req.tv_nsec = 50000000;
		nanosleep(&req, &req);
	}
	
	int i;

	/* Create data block to be inserted into buffer... */
	boost::shared_array<IpfixRecord::Data> htdata(new IpfixRecord::Data[fieldLength+privDataLength]);

	for (i = 0; i < dataTemplate->fieldCount; i++) {
		IpfixRecord::FieldInfo* hfi = &dataTemplate->fieldInfo[i];

		bool fieldFilled = false;
		
		/* Copy from matching variable field, should it exist */
		IpfixRecord::FieldInfo* tfi = ti->getFieldInfo(&hfi->type);
		if(tfi) {
			fieldFilled = true;
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
			fieldFilled = true;
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
		
		if (!fieldFilled) {
			// if field was not copied, fill it with 0
			memset(htdata.get() + hfi->offset, 0, hfi->type.length);
		}

		continue;
	}

	/* ...then buffer it */
	bufferDataBlock(htdata);
	
	atomic_release(&aggInProgress);
}
