#include "PacketHashtable.h"
#include <iostream>
#include <fstream>

#include "crc.hpp"

#include "ipfix.hpp"
#include "common/Misc.h"
#include "common/Time.h"
#include "HashtableBuckets.h"



PacketHashtable::PacketHashtable(Source<IpfixRecord*>* recordsource, Rule* rule,
		uint16_t minBufferTime, uint16_t maxBufferTime, uint8_t hashbits)
	: BaseHashtable(recordsource, rule, minBufferTime, maxBufferTime, hashbits),
	snapshotWritten(false), startTime(time(0))
{
	if (rule->biflowAggregation)
		THROWEXCEPTION("PacketAggregator can not perform biflow aggregation, but one of its rules is configured to do that");
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
void PacketHashtable::copyDataEqualLengthNoMod(IpfixRecord::Data* bucket, const IpfixRecord::Data* src, ExpFieldData* efd)
{
	IpfixRecord::Data* dst = bucket+efd->dstIndex;
	memcpy(dst, src, efd->srcLength);
}
void PacketHashtable::copyDataGreaterLengthIPNoMod(IpfixRecord::Data* bucket, const IpfixRecord::Data* src, ExpFieldData* efd)
{
	IpfixRecord::Data* dst = bucket+efd->dstIndex;
	bzero(dst+efd->srcLength, efd->dstLength-efd->srcLength);
	memcpy(dst, src, efd->srcLength);
}
void PacketHashtable::copyDataGreaterLengthNoMod(IpfixRecord::Data* bucket, const IpfixRecord::Data* src, ExpFieldData* efd)
{
	IpfixRecord::Data* dst = bucket+efd->dstIndex;
	bzero(dst, efd->dstLength-efd->srcLength);
	memcpy(dst+efd->dstLength-efd->srcLength, src, efd->srcLength);
}
void PacketHashtable::copyDataSetOne(IpfixRecord::Data* bucket, const IpfixRecord::Data* src, ExpFieldData* efd)
{
	IpfixRecord::Data* dst = bucket+efd->dstIndex;
	memset(dst, 0, efd->dstLength);
	// set last byte of array to one (network byte order!)
	dst[efd->dstLength-1] = 1;
}
void PacketHashtable::copyDataFrontPayload(IpfixRecord::Data* bucket, const IpfixRecord::Data* src, ExpFieldData* efd)
{
	aggregateFrontPayload(bucket, reinterpret_cast<const Packet*>(src), efd, true);
}
void PacketHashtable::copyDataMaxPacketGap(IpfixRecord::Data* bucket, const IpfixRecord::Data* src, ExpFieldData* efd)
{
	memset(bucket+efd->dstIndex, 0, efd->dstLength);
	memcpy(bucket+efd->privDataOffset, src, 8);
}
// copies data from given private data offset for this flow (is usually set to the private data element for the
// position pointer from copyDataFrontPayload
void PacketHashtable::copyDataFrontPayloadLen(IpfixRecord::Data* bucket, const IpfixRecord::Data* src, ExpFieldData* efd)
{
	IpfixRecord::Data* dst = bucket+efd->dstIndex;
	*reinterpret_cast<uint32_t*>(dst) = htonl(*reinterpret_cast<const uint32_t*>(bucket+efd->privDataOffset));
}
void PacketHashtable::copyDataNanoseconds(IpfixRecord::Data* bucket, const IpfixRecord::Data* src, ExpFieldData* efd)
{
	uint64_t ntptime;
	ntptime = ntp64timegcc(*reinterpret_cast<const struct timeval*>(src));
	uint64_t ntp2 = htonll(ntptime);
	DPRINTFL(MSG_VDEBUG, "ntp2: %llu, ntptime/ntp2 %llX/%llX", ntp2, ntptime, ntp2);
	memcpy(bucket+efd->dstIndex, &ntp2, sizeof(ntp2));
#ifdef DEBUG
	if (ntohll(*(uint64_t*)(bucket+efd->dstIndex))<(1000000000ULL+(2208988800ULL<<32)) || ntohll(*(uint64_t*)(bucket+efd->dstIndex))>(1300000000ULL+(2208988800ULL<<32))) {
		DPRINTFL(MSG_VDEBUG, "time before: %ds", reinterpret_cast<const struct timeval*>(src)->tv_sec);
		DPRINTFL(MSG_VDEBUG, "copy invalid end nano seconds: %lld s (%llX)", (ntohll(*(uint64_t*)(bucket+efd->dstIndex))>>32)-2208988800U, *(uint64_t*)(bucket+efd->dstIndex));
	}
#endif
}


/**
 * aggregates payload of packets to a certain maximum amount
 * only sequence number is regarded, succeeding packets with same sequence number
 * will overwrite data
 * ATTENTION: no stream reassembly is performed!
 */
void PacketHashtable::aggregateFrontPayload(IpfixRecord::Data* bucket, const Packet* src, const ExpFieldData* efd, bool firstpacket)
{
	IpfixRecord::Data* dst = bucket+efd->dstIndex;
	uint32_t seq = 0;
	if (src->ipProtocolType==Packet::TCP)
		seq = ntohl(*reinterpret_cast<const uint32_t*>(src->data+src->transportHeaderOffset+4));
	DPRINTFL(MSG_VDEBUG, "seq:%u, len:%u, udp:%u", seq, *reinterpret_cast<uint32_t*>(bucket+efd->privDataOffset+4), src->ipProtocolType==Packet::UDP);

	if (firstpacket) {
		if (src->ipProtocolType==Packet::TCP && src->data[src->transportHeaderOffset+13] & 0x02) {
			// SYN packet, so sequence number will be increased without any payload
			seq++;
		}
		// store sequence number and length of captured payload in private data
		*reinterpret_cast<uint32_t*>(bucket+efd->privDataOffset) = seq;
		*reinterpret_cast<uint32_t*>(bucket+efd->privDataOffset+4) = 0;
		memset(dst, 0, efd->dstLength);
	}

	// ignore packets that do either contain no payload or were not interpreted correctly
	if (src->payloadOffset==0 || src->payloadOffset==src->transportHeaderOffset) return;

	uint16_t plen = src->data_length-src->payloadOffset;

	if (plen>0) {
		uint32_t* pfplen = reinterpret_cast<uint32_t*>(bucket+efd->privDataOffset+4);

		if (src->ipProtocolType==Packet::TCP) {
			uint32_t fseq = *reinterpret_cast<const uint32_t*>(bucket+efd->privDataOffset);
			uint32_t fpos = *reinterpret_cast<const uint32_t*>(bucket+efd->privDataOffset+4);

			DPRINTFL(MSG_VDEBUG, "plen:%u, fseq:%u, seq:%u, dstleng:%u", plen, fseq, seq, efd->dstLength);

			if (seq-fseq<efd->dstLength) {
				uint32_t pos = (seq!=0 ? seq-fseq : fpos);
				uint32_t len = efd->dstLength-pos;
				if (plen<len) len = plen;
				DPRINTFL(MSG_VDEBUG, "inserting payload data at %u with length %u", pos, len);
				memcpy(dst+pos, src->data+src->payloadOffset, len);
				uint32_t maxpos = pos+len;
				if (*pfplen<maxpos) *pfplen = maxpos;
			}
		} else {
			uint32_t* pfplen = reinterpret_cast<uint32_t*>(bucket+efd->privDataOffset+4); // current size of front payload within flow
			if (*pfplen<efd->dstLength) {
				uint32_t len = efd->dstLength-*pfplen;
				if (plen<len) len = plen;
				memcpy(dst+*pfplen, src->data+src->payloadOffset, len);
				*pfplen += len;
			}
		}
	}
	DPRINTFL(MSG_VDEBUG, "new fplength: %u", *reinterpret_cast<uint32_t*>(bucket+efd->privDataOffset+4));
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
		case IPFIX_TYPEID_flowEndSysUpTime:
		case IPFIX_TYPEID_flowEndSeconds:
		case IPFIX_ETYPEID_frontPayloadLen:
		case IPFIX_ETYPEID_maxPacketGap:
			if (efd->dstLength != 4) {
				THROWEXCEPTION("unsupported length %d for type %d (\"%s\")", efd->dstLength, efd->typeId, typeid2string(efd->typeId));
			}
			break;

		case IPFIX_TYPEID_flowStartMilliSeconds:
		case IPFIX_TYPEID_flowStartMicroSeconds:
		case IPFIX_TYPEID_flowStartNanoSeconds:
		case IPFIX_TYPEID_flowEndMilliSeconds:
		case IPFIX_TYPEID_flowEndMicroSeconds:
		case IPFIX_TYPEID_flowEndNanoSeconds:
		case IPFIX_TYPEID_octetDeltaCount:
		case IPFIX_TYPEID_packetDeltaCount:
			if (efd->dstLength != 8) {
				THROWEXCEPTION("unsupported length %d for type %d (\"%s\")", efd->dstLength, efd->typeId, typeid2string(efd->typeId));
			}
			break;

		case IPFIX_TYPEID_sourceIPv4Address:
		case IPFIX_TYPEID_destinationIPv4Address:
			if (efd->dstLength != 5) {
				THROWEXCEPTION("unsupported length %d for type %d (\"%s\")", efd->dstLength, efd->typeId, typeid2string(efd->typeId));
			}
			break;
		case IPFIX_ETYPEID_frontPayload:
			if (efd->dstLength < 5) {
				THROWEXCEPTION("unsupported length %d for type %d (\"%s\")", efd->dstLength, efd->typeId, typeid2string(efd->typeId));
			}
			break;

		default:
			THROWEXCEPTION("type unhandled by Packet Aggregator: %d (\"%s\")", efd->typeId, typeid2string(efd->typeId));
			break;
	}

	// now decide on the correct copy function
	if (efd->typeId == IPFIX_ETYPEID_frontPayload) {
		return copyDataFrontPayload;
	} else if (efd->typeId == IPFIX_ETYPEID_frontPayloadLen) {
		return copyDataFrontPayloadLen;
	} else if (efd->typeId == IPFIX_ETYPEID_maxPacketGap) {
		return copyDataMaxPacketGap;
	} else if (efd->typeId == IPFIX_TYPEID_flowStartNanoSeconds || efd->typeId == IPFIX_TYPEID_flowEndNanoSeconds) {
		return copyDataNanoseconds;
	} else if (efd->dstLength == efd->srcLength) {
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
 * @returns field length in bytes of corresponding entry in raw packet
 **/
uint8_t PacketHashtable::getRawPacketFieldLength(IpfixRecord::FieldInfo::Type type)
{

	switch (type.id) {
		case IPFIX_TYPEID_protocolIdentifier:
		case IPFIX_TYPEID_tcpControlBits:
		case IPFIX_ETYPEID_revTcpControlBits:
		case IPFIX_TYPEID_packetDeltaCount:
		case IPFIX_ETYPEID_revPacketDeltaCount:
			return 1;

		case IPFIX_TYPEID_icmpTypeCode:
		case IPFIX_TYPEID_sourceTransportPort:
		case IPFIX_TYPEID_destinationTransportPort:
		case IPFIX_TYPEID_octetDeltaCount:
		case IPFIX_ETYPEID_revOctetDeltaCount:
			return 2;

		case IPFIX_TYPEID_flowStartSeconds:
		case IPFIX_TYPEID_flowEndSeconds:
		case IPFIX_TYPEID_sourceIPv4Address:
		case IPFIX_TYPEID_destinationIPv4Address:
		case IPFIX_ETYPEID_revFlowStartSeconds:
		case IPFIX_ETYPEID_revFlowEndSeconds:
		case IPFIX_ETYPEID_frontPayloadLen:
		case IPFIX_ETYPEID_maxPacketGap:
			return 4;

		case IPFIX_TYPEID_flowStartMilliSeconds:
		case IPFIX_TYPEID_flowEndMilliSeconds:
		case IPFIX_ETYPEID_revFlowStartMilliSeconds:
		case IPFIX_ETYPEID_revFlowEndMilliSeconds:
		case IPFIX_TYPEID_flowStartNanoSeconds:
		case IPFIX_TYPEID_flowEndNanoSeconds:
		case IPFIX_ETYPEID_revFlowStartNanoSeconds:
		case IPFIX_ETYPEID_revFlowEndNanoSeconds:
			return 8;

		case IPFIX_ETYPEID_frontPayload:
			return type.length;				// length is variable and is set in configuration

		default:
			THROWEXCEPTION("unknown typeid");
			break;
	}

	THROWEXCEPTION("unknown typeid");
	return 0;
}



/**
 * @returns if given field type is in varying positions inside a raw packet and inside the Packet structure
 */
bool PacketHashtable::isRawPacketPtrVariable(const IpfixRecord::FieldInfo::Type& type)
{
	switch (type.id) {
		case IPFIX_TYPEID_packetDeltaCount:
		case IPFIX_TYPEID_flowStartSeconds:
		case IPFIX_TYPEID_flowEndSeconds:
		case IPFIX_TYPEID_flowStartMilliSeconds: // those elements are inside the Packet structure, not in the raw packet.
		case IPFIX_TYPEID_flowEndMilliSeconds:   // nevertheless, we may access it relative to the start of the packet data
		case IPFIX_TYPEID_flowStartNanoSeconds: //  ^
		case IPFIX_TYPEID_flowEndNanoSeconds:   //  ^
		case IPFIX_TYPEID_octetDeltaCount:
		case IPFIX_TYPEID_protocolIdentifier:
		case IPFIX_TYPEID_sourceIPv4Address:
		case IPFIX_TYPEID_destinationIPv4Address:
		case IPFIX_ETYPEID_maxPacketGap:
			return false;

		case IPFIX_TYPEID_icmpTypeCode:
		case IPFIX_TYPEID_sourceTransportPort:
		case IPFIX_TYPEID_destinationTransportPort:
		case IPFIX_TYPEID_tcpControlBits:
		case IPFIX_ETYPEID_frontPayload:
		case IPFIX_ETYPEID_frontPayloadLen:
			return true;
	}

	THROWEXCEPTION("invalid type (%d)", type.id);
	return false;
}


/**
 * helper function for buildExpHelperTable
 */
void PacketHashtable::fillExpFieldData(ExpFieldData* efd, IpfixRecord::FieldInfo* hfi, Rule::Field::Modifier fieldModifier, uint16_t index)
{
	efd->typeId = hfi->type.id;
	efd->dstIndex = hfi->offset;
	efd->dstLength = hfi->type.length;
	efd->srcLength = getRawPacketFieldLength(hfi->type);
	efd->modifier = fieldModifier;
	efd->varSrcIdx = isRawPacketPtrVariable(hfi->type);
	efd->privDataOffset = hfi->privDataOffset;
	efd->fpLenOffset = 0;

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
 * @returns if given field is available in a raw ip packet
 */
bool PacketHashtable::typeAvailable(IpfixRecord::FieldInfo::Type type)
{
	switch (type.id) {
		case IPFIX_TYPEID_packetDeltaCount:
		case IPFIX_TYPEID_flowStartSeconds:
		case IPFIX_TYPEID_flowEndSeconds:
		case IPFIX_TYPEID_flowStartMilliSeconds:
		case IPFIX_TYPEID_flowEndMilliSeconds:
		case IPFIX_TYPEID_flowStartNanoSeconds:
		case IPFIX_TYPEID_flowEndNanoSeconds:
		case IPFIX_TYPEID_octetDeltaCount:
		case IPFIX_TYPEID_protocolIdentifier:
		case IPFIX_TYPEID_sourceIPv4Address:
		case IPFIX_TYPEID_destinationIPv4Address:
		case IPFIX_TYPEID_icmpTypeCode:
		case IPFIX_TYPEID_sourceTransportPort:
		case IPFIX_TYPEID_destinationTransportPort:
		case IPFIX_TYPEID_tcpControlBits:
		case IPFIX_ETYPEID_frontPayloadLen:
		case IPFIX_ETYPEID_frontPayload:
		case IPFIX_ETYPEID_maxPacketGap:
			return true;
	}

	return false;
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
		if (!typeAvailable(hfi->type)) {
			THROWEXCEPTION("Type '%s' is not contained in raw packet. Please remove it from PacketAggregator rule.", typeid2string(hfi->type.id));
		}
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
	expHelperTable.efdLength = efdIdx;

	// fill structure which contains field with variable pointers
	int noVarFields = 0;
	for (int i=0; i<expHelperTable.efdLength; i++) {
		if (expHelperTable.expFieldData[i].varSrcIdx) expHelperTable.varSrcPtrFields[noVarFields++] = i;
	}
	expHelperTable.varSrcPtrFieldsLen = noVarFields;
}


/**
 * calculates hash for given raw packet data in express aggregator
 */
uint32_t PacketHashtable::expCalculateHash(const IpfixRecord::Data* data)
{
	uint32_t hash = 0xAAAAAAAA;
	for (int i=expHelperTable.noAggFields; i<expHelperTable.efdLength; i++) {
		ExpFieldData* efd = &expHelperTable.expFieldData[i];
		hash = crc32(hash, efd->srcLength, reinterpret_cast<const char*>(data)+efd->srcIndex);
	}
	//return (hash>>(32-HTABLE_BITS)) & (HTABLE_SIZE-1);
	return hash & (htableSize-1);
}

/**
 * copies data from raw packet to a bucket which will be inserted into the hashtable
 * for aggregation (part of express aggregator)
 */
boost::shared_array<IpfixRecord::Data> PacketHashtable::buildBucketData(const Packet* p)
{
	// new field for insertion into hashtable
	boost::shared_array<IpfixRecord::Data> htdata(new IpfixRecord::Data[fieldLength+privDataLength]);
	IpfixRecord::Data* data = htdata.get();

	// copy all data ...
	for (int i=0; i<expHelperTable.efdLength; i++) {
		ExpFieldData* efd = &expHelperTable.expFieldData[i];
		efd->copyDataFunc(data, reinterpret_cast<const uint8_t*>(p->netHeader)+efd->srcIndex, efd);
	}

	return htdata;
}

/**
 * aggregates the given field of the raw packet data into a hashtable bucket
 * (part of express aggregator)
 */
void PacketHashtable::expAggregateField(const ExpFieldData* efd, IpfixRecord::Data* bucket, const IpfixRecord::Data* deltaData)
{
	IpfixRecord::Data* baseData = bucket+efd->dstIndex;
	int64_t gap;

	uint64_t ntptime;
	uint64_t ntp2;

	switch (efd->typeId) {
		case IPFIX_TYPEID_flowStartSeconds:
			*(uint32_t*)baseData = lesserUint32Nbo(*(uint32_t*)baseData, *(uint32_t*)deltaData);
			break;

		case IPFIX_TYPEID_flowStartMilliSeconds:
			*(uint64_t*)baseData = lesserUint64Nbo(*(uint64_t*)baseData, *(uint64_t*)deltaData);
			break;

		case IPFIX_TYPEID_flowStartNanoSeconds:
			ntptime = ntp64timegcc(*reinterpret_cast<const struct timeval*>(deltaData));
			ntp2 = htonll(ntptime);
			DPRINTFL(MSG_VDEBUG, "base: %lu s, delta: %lu s", (ntohll(*(uint64_t*)baseData)>>32)-2208988800U, ntohll(ntp2));
			DPRINTFL(MSG_VDEBUG, "base: %llX , delta: %llX", ntohll(*(uint64_t*)baseData), ntohll(ntp2));
			*(uint64_t*)baseData = lesserUint64Nbo(*(uint64_t*)baseData, ntp2);
#ifdef DEBUG
			if (ntohll(*(uint64_t*)baseData)<(1000000000ULL+(2208988800ULL<<32)) || ntohll(*(uint64_t*)baseData)>(1300000000ULL+(2208988800ULL<<32))) {
				DPRINTFL(MSG_VDEBUG, "invalid start nano seconds: %lu s", (ntohll(*(uint64_t*)baseData)>>32)-2208988800U);
				DPRINTFL(MSG_VDEBUG, "base: %llX , delta: %llX", *(uint64_t*)baseData, *(uint64_t*)deltaData);
			}
#endif
			break;

		case IPFIX_TYPEID_flowEndSeconds:
			*(uint32_t*)baseData = greaterUint32Nbo(*(uint32_t*)baseData, *(uint32_t*)deltaData);
			break;

		case IPFIX_TYPEID_flowEndMilliSeconds:
			*(uint64_t*)baseData = greaterUint64Nbo(*(uint64_t*)baseData, *(uint64_t*)deltaData);
			break;

		case IPFIX_TYPEID_flowEndNanoSeconds:
			ntptime = ntp64timegcc(*reinterpret_cast<const struct timeval*>(deltaData));
			ntp2 = htonll(ntptime);
			*(uint64_t*)baseData = greaterUint64Nbo(*(uint64_t*)baseData, ntp2);
#ifdef DEBUG
			if (ntohll(*(uint64_t*)baseData)<(1000000000ULL+(2208988800ULL<<32)) || ntohll(*(uint64_t*)baseData)>(1300000000ULL+(2208988800ULL<<32)))
				DPRINTFL(MSG_VDEBUG, "invalid end nano seconds: %lu s", (ntohll(*(uint64_t*)baseData)>>32)-2208988800U);
#endif
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

		case IPFIX_ETYPEID_frontPayload:
			aggregateFrontPayload(bucket, reinterpret_cast<const Packet*>(deltaData), efd);
			break;

		case IPFIX_ETYPEID_frontPayloadLen:
			*(uint32_t*)baseData = htonl(*reinterpret_cast<const uint32_t*>(bucket+efd->privDataOffset));
			break;

		case IPFIX_ETYPEID_maxPacketGap:
			gap = (int64_t)ntohll(*(int64_t*)deltaData)-(int64_t)ntohll(*reinterpret_cast<const uint64_t*>(bucket+efd->privDataOffset));
			if (gap<0) gap = -gap;
			DPRINTFL(MSG_VDEBUG, "gap: %u, oldgap: %u", gap, ntohl(*(uint32_t*)baseData));

			//msg(MSG_INFO, "gap: %u, oldgap: %u", gap, ntohl(*(uint32_t*)baseData));
			if ((uint32_t)gap > ntohl(*(uint32_t*)baseData)) *(uint32_t*)baseData = htonl(gap);
			*reinterpret_cast<uint64_t*>(bucket+efd->privDataOffset) = *(uint64_t*)deltaData;
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

		expAggregateField(efd, bucket, p->netHeader+efd->srcIndex);
	}
}

/**
 * compares if given hashtable bucket data is equal with raw packet data
 * (part of express aggregator)
 * @returns true if equal, false if not equal
 */
bool PacketHashtable::expEqualFlow(IpfixRecord::Data* bucket, const Packet* p)
{
	for (int i=expHelperTable.noAggFields; i<expHelperTable.efdLength; i++) {
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
void PacketHashtable::createMaskedField(IpfixRecord::Data* address, uint8_t imask)
{
	DPRINTF("unmasked address: %08X", *reinterpret_cast<uint32_t*>(address));
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
			// tobi_optimize: do this mask calculation during initialization phase of express aggregator
			int pattern = 0;
			int i;
			for(i = 0; i < imask; i++) {
				pattern |= (1 << i);
			}
			*(uint32_t*)address = htonl(ntohl(*(uint32_t*)(address)) & ~pattern);
		}
	}
	DPRINTF("masked address: %08X", *reinterpret_cast<uint32_t*>(address));
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


		switch (efd->typeId) {
			// perform a hack for masked IPs:
			// IP addresses which are to be masked are copied to efd->data[0-3] and masked there
			// now we need to do some pointer arithmetic to be able to access those transparently afterwards
			// note: only IP types to be masked have efd->varSrcIdx set
			case IPFIX_TYPEID_destinationIPv4Address:
			case IPFIX_TYPEID_sourceIPv4Address:
				efd->srcIndex = reinterpret_cast<uintptr_t>(&efd->data[0])-reinterpret_cast<uintptr_t>(p->netHeader);
				break;

			// aggregation and copy functions for frontPayload need to have source pointer
			// pointing to packet structure
			case IPFIX_ETYPEID_frontPayload:
				efd->srcIndex = reinterpret_cast<uintptr_t>(p)-reinterpret_cast<uintptr_t>(p->netHeader);
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
	// the following lock should almost never block (only during reconfiguration)
	while (atomic_lock(&aggInProgress)) {
		timespec req;
		req.tv_sec = 0;
		req.tv_nsec = 50000000;
		nanosleep(&req, &req);
	}

	DPRINTF("PacketHashtable::aggregatePacket()");
	updatePointers(p);
	createMaskedFields(p);

	uint32_t hash = expCalculateHash(p->netHeader);

	// search bucket inside hashtable
	HashtableBucket* bucket = buckets[hash];
	if (bucket == 0) {
		// slot is free, place bucket there
		DPRINTF("creating new bucket");
		buckets[hash] = createBucket(buildBucketData(p), p->observationDomainID, 0, 0, hash);
		BucketListElement* node = hbucketIM.getNewInstance();
		node->reset();
		node->bucket = buckets[hash];
		buckets[hash]->listNode = node;
		exportList.push(node);
		statTotalEntries++;
	} else {
		// This slot is already used, search spill chain for equal flow
		while(1) {
			if (expEqualFlow(bucket->data.get(), p)) {
				DPRINTF("appending to bucket\n");

				expAggregateFlow(bucket->data.get(), p);

				// TODO: tobi_optimize
				// replace call of time() with access to a static variable which is updated regularly (such as every 100ms)
				bucket->expireTime = time(0) + minBufferTime;

				if (bucket->forceExpireTime>bucket->expireTime) {
					exportList.remove(bucket->listNode);
					exportList.push(bucket->listNode);
				}
				break;
			}

			if (bucket->next == 0) {
				DPRINTF("creating bucket\n");
				statTotalEntries++;
				statMultiEntries++;
				HashtableBucket* buck = createBucket(buildBucketData(p), p->observationDomainID, 0, bucket, hash);
				bucket->next = buck;
				BucketListElement* node = hbucketIM.getNewInstance();
				node->reset();
				exportList.push(node);
				node->bucket = buck;
				buck->listNode = node;
				break;
			}
			bucket = (HashtableBucket*)bucket->next;
		}
	}
	//if (!snapshotWritten && (time(0)- 300 > starttime)) writeHashtable();
	// FIXME: enable snapshots again by configuration
	atomic_release(&aggInProgress);
}

void PacketHashtable::snapshotHashtable()
{
	// FIXME: this snapshotting code is not good ...
	int count = 0;
	ofstream fout("/home/sistmika/vermont/dos-attack/hashtable.txt");
	if (fout){

		fout << "bucket\tnumber\n";
		for(uint32_t i = 0; i < htableSize; i++){
			HashtableBucket* bucket = buckets[i];
			if (bucket == 0) count = 0;
			else{
				count++;
				while(bucket->next != 0){
					count++;
					bucket = (HashtableBucket*)bucket->next;
					}
			}
			fout << i+1 << "\t" << count  << "\n";
			count =0;
		}
		fout.close();
		snapshotWritten = true;
	}
	else {
	DPRINTF("unable to open file to write Hashtable\n");
	}
}

