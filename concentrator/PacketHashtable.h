#ifndef PACKETHASHTABLE_H_
#define PACKETHASHTABLE_H_


#include "Rule.hpp"
#include "sampler/Packet.h"
#include "BaseHashtable.h"

#include <boost/smart_ptr.hpp>



class PacketHashtable : public BaseHashtable
{
public:
	PacketHashtable(Source<IpfixRecord*>* recordsource, Rule* rule, 
			uint16_t minBufferTime, uint16_t maxBufferTime);
	virtual ~PacketHashtable();
	
	void aggregatePacket(const Packet* p);
	

private:
	/** 
	 * fast accessible structure containing data for aggregation, the first noAggFields members of array
	 * are aggregatable
	 */
	struct ExpFieldData
	{
		uint16_t typeId; /**< type of corresponding ipfix field */

		// following fields are used by aggregation functions for a fast lookup of needed data inside
		// the raw packet (source) and the hashtable bucket (destination)
		uint32_t srcIndex; /**< index to raw packet data relative to Packet::netHeader, sometimes unique for each processed packet */
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

		void (*copyDataFunc)(IpfixRecord::Data*, const IpfixRecord::Data*, ExpFieldData*); /**< function which is able to copy data from raw packet to ipfix field */

	};
	struct ExpHelperTable
	{
		/**< contains number of aggregatable fields in expFieldData */
		uint16_t noAggFields;

		uint16_t dstIpEFieldIndex; /**< 0 if destination ip should not be masked, == index dstip, if to be masked */
		uint16_t srcIpEFieldIndex; /**< 0 if source ip should not be masked, == index srcip, if to be masked */

		ExpFieldData* expFieldData;
		uint16_t efdLength;

		uint16_t* varSrcPtrFields; /**< array with indizes to expFieldData elements, which have a srcIndex which varies from packet to packet */
		uint16_t varSrcPtrFieldsLen; /**< length of varSrcPtrFields */
		
	};

	ExpHelperTable expHelperTable;
	
	void buildExpHelperTable();
	static void copyDataEqualLengthNoMod(IpfixRecord::Data* bucket, const IpfixRecord::Data* src, ExpFieldData* efd);
	static void copyDataGreaterLengthIPNoMod(IpfixRecord::Data* bucket, const IpfixRecord::Data* src, ExpFieldData* efd);
	static void copyDataGreaterLengthIPMask(IpfixRecord::Data* bucket, const IpfixRecord::Data* src, ExpFieldData* efd);
	static void copyDataGreaterLengthNoMod(IpfixRecord::Data* bucket, const IpfixRecord::Data* src, ExpFieldData* efd);
	static void copyDataFrontPayload(IpfixRecord::Data* bucket, const IpfixRecord::Data* src, ExpFieldData* efd);
	static void copyDataSetOne(IpfixRecord::Data* bucket, const IpfixRecord::Data* src, ExpFieldData* efd);
	static void aggregateFrontPayload(IpfixRecord::Data* bucket, const Packet* src, const ExpFieldData* efd, bool firstpacket = false);
	void (*getCopyDataFunction(const ExpFieldData* efd))(IpfixRecord::Data*, const IpfixRecord::Data*, ExpFieldData*);
	void fillExpFieldData(ExpFieldData* efd, IpfixRecord::FieldInfo* hfi, Rule::Field::Modifier fieldModifier, uint16_t index);
	uint32_t expCalculateHash(const IpfixRecord::Data* data);
	boost::shared_array<IpfixRecord::Data> buildBucketData(const Packet* p);
	void expAggregateField(const ExpFieldData* efd, IpfixRecord::Data* baseData, const IpfixRecord::Data* deltaData);
	void expAggregateFlow(IpfixRecord::Data* bucket, const Packet* p);
	bool expEqualFlow(IpfixRecord::Data* bucket, const Packet* p);
	void createMaskedField(IpfixRecord::Data* address, uint8_t imask);
	void createMaskedFields(const Packet* p);
	void updatePointers(const Packet* p);
	bool typeAvailable(IpfixRecord::FieldInfo::Type type);
	uint8_t getRawPacketFieldLength(IpfixRecord::FieldInfo::Type type);
	uint16_t getRawPacketFieldIndex(uint16_t typeId, const Packet* p);
	bool isRawPacketPtrVariable(const IpfixRecord::FieldInfo::Type& type);
	
};

#endif /*PACKETHASHTABLE_H_*/
