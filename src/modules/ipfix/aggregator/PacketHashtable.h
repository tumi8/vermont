/*
 * Vermont Aggregator Subsystem
 * Copyright (C) 2009 Vermont Project
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

#ifndef PACKETHASHTABLE_H_
#define PACKETHASHTABLE_H_


#include "Rule.hpp"
#include "modules/packet/Packet.h"
#include "BaseHashtable.h"
#include <iostream>
#include <fstream>

#include <boost/smart_ptr.hpp>



class PacketHashtable : public BaseHashtable
{
public:
	PacketHashtable(Source<IpfixRecord*>* recordsource, Rule* rule,
			uint16_t minBufferTime, uint16_t maxBufferTime, uint8_t hashbits);
	virtual ~PacketHashtable();

	void aggregatePacket(Packet* p);

	static uint8_t getRawPacketFieldLength(const InformationElement::IeInfo& type);
	static int32_t getRawPacketFieldOffset(const InformationElement::IeInfo& type, const Packet* p);

private:
	/**
	 * contains all parameters that are passed to the copy functions for new flows
	 */
	struct ExpFieldData;
	struct CopyFuncParameters {
		const Packet* packet;
		IpfixRecord::Data* dst;
		IpfixRecord::Data* src;
		ExpFieldData* efd;
	};

	/**
	 * fast accessible structure containing data for aggregation, the first noAggFields members of array
	 * are aggregatable
	 */
	struct ExpFieldData
	{
		InformationElement::IeInfo typeId; /**< type of corresponding ipfix field */

		// following fields are used by aggregation functions for a fast lookup of needed data inside
		// the raw packet (source) and the hashtable bucket (destination)
		intptr_t srcIndex; /**< index to raw packet data relative to Packet::netHeader, sometimes unique for each processed packet */
		uint16_t dstIndex; /**< index in ipfix data */
		uint16_t srcLength; /**< length of source field data */
		uint16_t dstLength; /**< length of destination field data */

		/**
		 * additional data stored by aggregation function
		 * if ip addresses need to be masked, this contains the masked ips (as the raw packet data must not
		 * be touched) + mask byte
		 */
		uint8_t data[5];

		/**
		 * this index is used by the createMaskedField function to determine original location of IP address
		 * inside the raw packet (as srcIndex is overwritten with index which points to data[0]
		 */
		uint32_t origSrcIndex;

		bool varSrcIdx; /**< specifies if the index in the raw packet data is variable between packets relative to Packet::netHeader*/

		Rule::Field::Modifier modifier; /**< modifier when copying field (such as a mask) */

		uint32_t privDataOffset; /**< offset for private data inside flow, if available */

		void (*copyDataFunc)(CopyFuncParameters*); /**< function which is able to copy data from raw packet to ipfix field */

		/**
		 * contains type specific data that is needed for aggregation
		 */
		union TypeSpecificData {
			struct FrontPayloadData {
				uint32_t pktCountOffset; /**< offset from start of record data to IPFIX_ETYPE_FRONTPAYLOADPKTCOUNT, 0xFFFFFFFF if not used */
				uint32_t dpaForcedExportOffset; /**< for DPA: offset from start of record data to IPFIX_ETYPE_DPAFORCEDEXPORT, ::UNUSED if not used */
				uint32_t dpaPrivDataOffset; /**< for DPA: offset from start of record data to private data of IPFIX_ETYPE_DPAFORCEDEXPORT, ::UNUSED if not used */
				uint32_t dpaRevStartOffset; /**< for DPA: offset from start of record data to IPFIX_ETYPE_DPAREVERSESTART, ::UNUSED if not used */
				uint32_t fpaLenOffset; /**< offset from start of record data to IPFIX_ETYPE_FRONTPAYLOADLEN, 0xFFFFFFFF if not used */
				bool dpa; /**< set to true, if DPA was activated */
			} frontPayload;
		} typeSpecData;

	};
	struct ExpHelperTable
	{
		uint16_t dstIpEFieldIndex; /**< 0 if destination ip should not be masked, == index dstip, if to be masked */
		uint16_t srcIpEFieldIndex; /**< 0 if source ip should not be masked, == index srcip, if to be masked */

		ExpFieldData* aggFields;
		uint16_t noAggFields; /**< contains number of aggregatable fields in expFieldData (excluded reverse fields) */
		ExpFieldData* revAggFields;
		uint16_t noRevAggFields; /**< contains number of reverse aggregatable fields in expFieldData */
		ExpFieldData* keyFields;
		uint16_t noKeyFields; /**< contains number of key fields in expFieldData */

		vector<ExpFieldData*> allFields; /** all fields mapped in one structure */

		ExpFieldData** varSrcPtrFields; /**< array with indizes to expFieldData elements, which have a srcIndex which varies from packet to packet */
		uint16_t noVarSrcPtrFields;
		ExpFieldData** revKeyFieldMapper; /**< maps field indizes to their reverse indizes */
		bool useDPA; /**< set to true when DPA is used for front payload aggregation */
		uint32_t dpaFlowCountOffset; /**< for DPA: offset from start of record data to IPFIX_ETYPE_DPAFLOWCOUNT (number of switched dialogues), ::UNUSED if not used */

		static const uint32_t UNUSED;
	};

	ExpHelperTable expHelperTable;

	bool snapshotWritten; /**< set to true, if snapshot of hashtable was already written */

	void snapshotHashtable();
	void buildExpHelperTable();

	static void copyDataEqualLengthNoMod(CopyFuncParameters* cfp);
	static void copyDataGreaterLengthIPNoMod(CopyFuncParameters* cfp);
	static void copyDataGreaterLengthIPMask(CopyFuncParameters* cfp);
	static void copyDataGreaterLengthNoMod(CopyFuncParameters* cfp);
	static void copyDataFrontPayload(CopyFuncParameters* cfp);
	static void copyDataFrontPayloadNoInit(CopyFuncParameters* cfp);
	static void copyDataSetOne(CopyFuncParameters* cfp);
	static void copyDataSetZero(CopyFuncParameters* cfp);
	static void copyDataMaxPacketGap(CopyFuncParameters* cfp);
	static void copyDataNanoseconds(CopyFuncParameters* cfp);
	static void copyDataDummy(CopyFuncParameters* cfp);
	static void copyDataTransportOctets(CopyFuncParameters* cfp);
	static void aggregateFrontPayload(IpfixRecord::Data* bucket, HashtableBucket* hbucket, const Packet* src,
									  const ExpFieldData* efd, bool firstpacket, bool onlyinit);
	void (*getCopyDataFunction(const ExpFieldData* efd))(CopyFuncParameters*);
	void fillExpFieldData(ExpFieldData* efd, TemplateInfo::FieldInfo* hfi, Rule::Field::Modifier fieldModifier, uint16_t index);
	uint32_t calculateHash(const IpfixRecord::Data* data);
	uint32_t calculateHashRev(const IpfixRecord::Data* data);
	boost::shared_array<IpfixRecord::Data> buildBucketData(Packet* p);
	void aggregateField(const ExpFieldData* efd, HashtableBucket* hbucket,
					    const IpfixRecord::Data* deltaData, IpfixRecord::Data* data);
	void aggregateFlow(HashtableBucket* bucket, const Packet* p, bool reverse);
	bool equalFlow(IpfixRecord::Data* bucket, const Packet* p);
	bool equalFlowRev(IpfixRecord::Data* bucket, const Packet* p);
	void createMaskedField(IpfixRecord::Data* address, uint8_t imask);
	void createMaskedFields( Packet* p);
	void updatePointers(const Packet* p);
	bool typeAvailable(const InformationElement::IeInfo& type);
	bool isRawPacketPtrVariable(const InformationElement::IeInfo& type);
	void updateBucketData(HashtableBucket* bucket);
	uint32_t getDstOffset(const InformationElement::IeInfo& ietype);
	bool mustExpireBucket(const HashtableBucket* bucket, const Packet* p);

};

#endif /*PACKETHASHTABLE_H_*/
