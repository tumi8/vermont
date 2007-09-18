#include "PacketHashtable.h"

#include "crc.hpp"

#include "common/Misc.h"

PacketHashtable::PacketHashtable(Source<IpfixRecord*>* recordsource, Rule* rule, uint16_t minBufferTime, uint16_t maxBufferTime)
	: BaseHashtable(recordsource, rule, minBufferTime, maxBufferTime)
{
	buildExpHelperTable();
}


PacketHashtable::~PacketHashtable()
{
	// free express aggregator helper structures
	delete[] expHelperTable.expFieldData;
	delete[] expHelperTable.varSrcPtrFields;
}

/** 
 * copy functions which were extracted from ExpcopyData
 * those copy data from the original raw packet into the ipfix bucket in the hashtable
 * (always called, when a new bucket has to be created for a new flow)
 */
void PacketHashtable::copyDataEqualLengthNoMod(IpfixRecord::Data* dst, const IpfixRecord::Data* src, ExpFieldData* efd)
{
	memcpy(dst, src, efd->srcLength);
}
void PacketHashtable::copyDataGreaterLengthIPNoMod(IpfixRecord::Data* dst, const IpfixRecord::Data* src, ExpFieldData* efd)
{
	bzero(dst+efd->srcLength, efd->dstLength-efd->srcLength);
	memcpy(dst, src, efd->srcLength);
}
void PacketHashtable::copyDataGreaterLengthNoMod(IpfixRecord::Data* dst, const IpfixRecord::Data* src, ExpFieldData* efd)
{
	bzero(dst, efd->dstLength-efd->srcLength);
	memcpy(dst+efd->dstLength-efd->srcLength, src, efd->srcLength);
}
void PacketHashtable::copyDataSetOne(IpfixRecord::Data* dst, const IpfixRecord::Data* src, ExpFieldData* efd)
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
void (*PacketHashtable::getCopyDataFunction(const ExpFieldData* efd))(IpfixRecord::Data*, const IpfixRecord::Data*, ExpFieldData*)
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
				THROWEXCEPTION("unsupported length %d for type %d (\"%s\")", efd->dstLength, efd->typeId, typeid2string(efd->typeId));
			}
			break;

		case IPFIX_TYPEID_sourceTransportPort:
		case IPFIX_TYPEID_destinationTransportPort:
		case IPFIX_TYPEID_icmpTypeCode:
			if (efd->dstLength != 2) {
				THROWEXCEPTION("unsupported length %d for type %d (\"%s\")", efd->dstLength, efd->typeId, typeid2string(efd->typeId));
			}
			break;

		case IPFIX_TYPEID_flowStartSysUpTime:
		case IPFIX_TYPEID_flowStartSeconds:
		case IPFIX_TYPEID_flowStartMicroSeconds:
		case IPFIX_TYPEID_flowStartNanoSeconds:
			if (efd->dstLength != 4) {
				THROWEXCEPTION("unsupported length %d for type %d (\"%s\")", efd->dstLength, efd->typeId, typeid2string(efd->typeId));
			}
			break;

		case IPFIX_TYPEID_flowStartMilliSeconds:
			if (efd->dstLength != 8) {
				THROWEXCEPTION("unsupported length %d for type %d (\"%s\")", efd->dstLength, efd->typeId, typeid2string(efd->typeId));
			}
			break;

		case IPFIX_TYPEID_flowEndSysUpTime:
		case IPFIX_TYPEID_flowEndSeconds:
		case IPFIX_TYPEID_flowEndMicroSeconds:
		case IPFIX_TYPEID_flowEndNanoSeconds:
			if (efd->dstLength != 4) {
				THROWEXCEPTION("unsupported length %d for type %d (\"%s\")", efd->dstLength, efd->typeId, typeid2string(efd->typeId));
			}
			break;

		case IPFIX_TYPEID_flowEndMilliSeconds:
			if (efd->dstLength != 8) {
				THROWEXCEPTION("unsupported length %d for type %d (\"%s\")", efd->dstLength, efd->typeId, typeid2string(efd->typeId));
			}
			break;

		case IPFIX_TYPEID_octetDeltaCount:
			if (efd->dstLength != 8) {
				THROWEXCEPTION("unsupported length %d for type %d (\"%s\")", efd->dstLength, efd->typeId, typeid2string(efd->typeId));
			}

		case IPFIX_TYPEID_packetDeltaCount:
			switch (efd->dstLength) {
				case 1:
				case 2:
				case 4:
				case 8:
					break;

				default:
					THROWEXCEPTION("unsupported length %d for type %d (\"%s\")", efd->dstLength, efd->typeId, typeid2string(efd->typeId));
					break;
			}
			break;

		case IPFIX_TYPEID_sourceIPv4Address:
		case IPFIX_TYPEID_destinationIPv4Address:
			if (efd->dstLength != 5) {
				THROWEXCEPTION("unsupported length %d for type %d (\"%s\")", efd->dstLength, efd->typeId, typeid2string(efd->typeId));
			}
			break;

		default:
			THROWEXCEPTION("type unhandled by Packet Aggregator: %d (\"%s\")", efd->typeId, typeid2string(efd->typeId));
			break;
	}

	// now decide on the correct copy function
	if (efd->dstLength == efd->srcLength) {
		return copyDataEqualLengthNoMod;
	} else if (efd->dstLength > efd->srcLength) {
		if (efd->typeId == IPFIX_TYPEID_sourceIPv4Address || efd->typeId == IPFIX_TYPEID_destinationIPv4Address) {
			if ((efd->modifier >= Rule::Field::MASK_START) && (efd->modifier <= Rule::Field::MASK_END)) {
				if (efd->dstLength != 5 && efd->srcLength != 5) {
					THROWEXCEPTION("destination and source data size must be 5, but is %d/%d - mask needs to be stored in both of them", efd->dstLength, efd->srcLength);
				}
				return copyDataEqualLengthNoMod;
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
void PacketHashtable::fillExpFieldData(ExpFieldData* efd, IpfixRecord::FieldInfo* hfi, Rule::Field::Modifier fieldModifier, uint16_t index)
{
	efd->typeId = hfi->type.id;
	efd->dstIndex = hfi->offset;
	efd->srcLength = IpfixRecord::TemplateInfo::getFieldLength(hfi->type);
	efd->dstLength = hfi->type.length;
	efd->modifier = fieldModifier;
	efd->varSrcIdx = IpfixRecord::TemplateInfo::isRawPacketPtrVariable(hfi->type);

	// initialize static source index, if current field does not have a variable pointer
	if (!efd->varSrcIdx) {
		Packet p; // not good: create temporary packet just for initializing our optimization structure
		efd->srcIndex = IpfixRecord::TemplateInfo::getRawPacketFieldIndex(hfi->type.id, &p);
	}

	// special case for masked IPs: those contain variable pointers, if they are masked
	if ((efd->typeId==IPFIX_TYPEID_sourceIPv4Address || efd->typeId==IPFIX_TYPEID_destinationIPv4Address) &&
			(efd->modifier >= Rule::Field::MASK_START) && (efd->modifier <= Rule::Field::MASK_END)) {

		// ok, our dst/src ip has to be masked, so this is a variable pointer
		efd->varSrcIdx = true;
		// calculate inverse network mask using the modifier
		efd->data[4] = 32 - (efd->modifier - (int)Rule::Field::MASK_START); 
		// save index of srcIndex, as this variable is overwritten by updatePointers for each packet
		efd->origSrcIndex = efd->srcIndex;

		switch (efd->typeId) {
			case IPFIX_TYPEID_sourceIPv4Address:
				expHelperTable.srcIpEFieldIndex = index;
				break;
			case IPFIX_TYPEID_destinationIPv4Address:
				expHelperTable.dstIpEFieldIndex = index;
				break;
		}

		// adjust srcLength, as our source length is 5 bytes including the appended mask!
		efd->srcLength = 5;
	}

	efd->copyDataFunc = getCopyDataFunction(efd);

}

/**
 * builds internal structure expHelperTable for fast aggregation of raw packets
 * used in the express aggregator
 */
void PacketHashtable::buildExpHelperTable()
{
	expHelperTable.expFieldData = new ExpFieldData[dataTemplate->fieldCount];
	expHelperTable.varSrcPtrFields = new uint16_t[dataTemplate->fieldCount];

	// TODO: fill with correct value!
	expHelperTable.dstIpEFieldIndex = false;
	expHelperTable.srcIpEFieldIndex = false;

	// at first, fill data structure with aggregatable fields
	uint16_t efdIdx = 0;
	for (int i=0; i<dataTemplate->fieldCount; i++) {
		IpfixRecord::FieldInfo* hfi = &dataTemplate->fieldInfo[i];
		if (!isToBeAggregated(hfi->type)) continue;
		ExpFieldData* efd = &expHelperTable.expFieldData[efdIdx++];
		fillExpFieldData(efd, hfi, fieldModifier[i], efdIdx-1);
	}
	expHelperTable.noAggFields = efdIdx;

	// now the remaining fields
	for (int i=0; i<dataTemplate->fieldCount; i++) {
		IpfixRecord::FieldInfo* hfi = &dataTemplate->fieldInfo[i];
		if (isToBeAggregated(hfi->type)) continue;
		ExpFieldData* efd = &expHelperTable.expFieldData[efdIdx++];
		fillExpFieldData(efd, hfi, fieldModifier[i], efdIdx-1);
	}

	// fill structure which contains field with variable pointers
	int noVarFields = 0;
	for (int i=0; i<dataTemplate->fieldCount; i++) {
		if (expHelperTable.expFieldData[i].varSrcIdx) expHelperTable.varSrcPtrFields[noVarFields++] = i;
	}
	expHelperTable.varSrcPtrFieldsLen = noVarFields;
}


/**
 * calculates hash for given raw packet data in express aggregator
 */
uint16_t PacketHashtable::expCalculateHash(const IpfixRecord::Data* data)
{
	uint32_t hash = 0;
	for (int i=expHelperTable.noAggFields; i<dataTemplate->fieldCount; i++) {
		ExpFieldData* efd = &expHelperTable.expFieldData[i];
		hash = crc32(hash, efd->srcLength, reinterpret_cast<const char*>(data)+efd->srcIndex);
	}
	//return (hash>>(32-HTABLE_BITS)) & (HTABLE_SIZE-1);
	return hash & (HTABLE_SIZE-1);
}

/**
 * copies data from raw packet to a bucket which will be inserted into the hashtable
 * for aggregation (part of express aggregator)
 */
boost::shared_array<IpfixRecord::Data> PacketHashtable::buildBucketData(const Packet* p)
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
void PacketHashtable::expAggregateField(const ExpFieldData* efd, IpfixRecord::Data* baseData, const IpfixRecord::Data* deltaData)
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
void PacketHashtable::expAggregateFlow(IpfixRecord::Data* bucket, const Packet* p)
{
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
bool PacketHashtable::expEqualFlow(IpfixRecord::Data* bucket, const Packet* p)
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
 * masks ip addresses if desired in ExpFieldData->[0-3]
 * additional mask information (is 5th byte in aggregated data) is in ExpFieldData->data[4]
 */
void PacketHashtable::createMaskedFields(const Packet* p)
{
	if (expHelperTable.dstIpEFieldIndex > 0) {
		ExpFieldData* efd = &expHelperTable.expFieldData[expHelperTable.dstIpEFieldIndex];
		// copy *original* ip address in *raw packet* to our temporary structure
		*reinterpret_cast<uint32_t*>(&efd->data[0]) = *reinterpret_cast<uint32_t*>(p->netHeader+efd->origSrcIndex);
		// then mask it
		createMaskedField(&efd->data[0], efd->data[4]);
	}
	if (expHelperTable.srcIpEFieldIndex > 0) {
		ExpFieldData* efd = &expHelperTable.expFieldData[expHelperTable.srcIpEFieldIndex];
		// copy *original* ip address in *raw packet* to our temporary structure
		*reinterpret_cast<uint32_t*>(&efd->data[0]) = *reinterpret_cast<uint32_t*>(p->netHeader+efd->origSrcIndex);
		// then mask it
		createMaskedField(&efd->data[0], efd->data[4]);
	}
}

/**
 * updates variable pointers to the raw packet data for each packet
 * (part of express aggregator)
 */
void PacketHashtable::updatePointers(const Packet* p)
{
	for (int i=0; i<expHelperTable.varSrcPtrFieldsLen; i++) {
		ExpFieldData* efd = &expHelperTable.expFieldData[expHelperTable.varSrcPtrFields[i]];

		// perform a hack for masked IPs:
		// IP addresses which are to be masked are copied to efd->data[0-3] and masked there
		// now we need to do some pointer arithmetic to be able to access those transparently afterwards
		// note: only IP types to be masked have efd->varSrcIdx set
		switch (efd->typeId) {
			case IPFIX_TYPEID_destinationIPv4Address:
			case IPFIX_TYPEID_sourceIPv4Address:
				efd->srcIndex = reinterpret_cast<uint32_t>(&efd->data[0])-reinterpret_cast<uint32_t>(p->netHeader);
				break;

			default:
				// standard procedure for transport header fields
				efd->srcIndex = IpfixRecord::TemplateInfo::getRawPacketFieldIndex(efd->typeId, p);
				break;
		}
	}
}

/**
 * Replacement for ExpAggregateTemplateData
 * inserts the given raw packet into the hashtable
 * ATTENTION: 
 *  - this function expects not to be called in parallel, as it uses internal buffers which are
 *    *NOT* thread-safe
 *  - hashes are calculated based on raw packet (masks are already applied then)
 */
void PacketHashtable::aggregatePacket(const Packet* p)
{
	updatePointers(p);
	createMaskedFields(p);

	uint16_t hash = expCalculateHash(p->netHeader);

	// search bucket inside hashtable
	Bucket* bucket = buckets[hash];
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
		bucket = (Bucket*)bucket->next;
	}
}
