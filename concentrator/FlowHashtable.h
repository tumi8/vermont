#ifndef FLOWHASHTABLE_H_
#define FLOWHASHTABLE_H_


#include "BaseHashtable.h"

#include <boost/smart_ptr.hpp>
#include <map>

class FlowHashtable : public BaseHashtable
{
public:
	FlowHashtable(Source<IpfixRecord*>* recordsource, Rule* rule,
			uint16_t minBufferTime, uint16_t maxBufferTime);
	virtual ~FlowHashtable();

	void aggregateTemplateData(IpfixRecord::TemplateInfo* ti, IpfixRecord::Data* data);
	void aggregateDataTemplateData(IpfixRecord::DataTemplateInfo* ti, IpfixRecord::Data* data);


private:
	uint32_t* revDataTemplateMapper; /**< contains indizes to dataTemplate for a reverse flow*/
	bool biflowAggregation; /**< set to true if biflow aggregation is to be done*/

	/**
	 * contain indizes to reverse aggregatable fields:
	 *
	 * <typeid, index to dataTemplate>
	 *
	 * IPFIX_ETYPEID_revFlowStartSeconds
	 * IPFIX_ETYPEID_revFlowStartMilliSeconds
	 * IPFIX_ETYPEID_revFlowEndSeconds
	 * IPFIX_ETYPEID_revFlowEndMilliSeconds
	 * IPFIX_ETYPEID_revOctetDeltaCount
	 * IPFIX_ETYPEID_revPacketDeltaCount
	 * IPFIX_ETYPEID_revTcpControlBits
	 */
	map<uint32_t, uint32_t> mapRevAggIndizes;

	/**
	 * contains indizes to reversed flow fields
	 * e.g. srcIP index points to dstIP index, and dstIP index points to srcIP index
	 */
	vector<uint32_t> flowReverseMapper;
	char* switchArray; /**< used by function reverseFlowBucket as temporary storage */

	int aggregateField(IpfixRecord::FieldInfo* basefi, IpfixRecord::FieldInfo* deltafi, IpfixRecord::Data* base,
		  IpfixRecord::Data* delta);
	int aggregateFlow(IpfixRecord::Data* baseFlow, IpfixRecord::Data* flow, bool reverse);
	uint32_t getHash(IpfixRecord::Data* data, bool reverse);
	int equalFlow(IpfixRecord::Data* flow1, IpfixRecord::Data* flow2, bool reverse);
	Bucket* lookupBucket(uint32_t hash, IpfixRecord::Data* data, bool reverse, Bucket** prevbucket);
	void bufferDataBlock(boost::shared_array<IpfixRecord::Data> data);
	int equalRaw(IpfixRecord::FieldInfo::Type* data1Type, IpfixRecord::Data* data1,
			IpfixRecord::FieldInfo::Type* data2Type, IpfixRecord::Data* data2);
	void copyData(IpfixRecord::FieldInfo* dstFI, IpfixRecord::Data* dst,
			IpfixRecord::FieldInfo* srcFI, IpfixRecord::Data* src, Rule::Field::Modifier modifier);
	void genBiflowStructs();
	int compare4ByteField(IpfixRecord::Data* baseFlow, IpfixRecord::FieldInfo* baseFi,
			IpfixRecord::Data* flow, IpfixRecord::FieldInfo* deltaFi);
	int compare8ByteField(IpfixRecord::Data* baseFlow, IpfixRecord::FieldInfo* baseFi,
			IpfixRecord::Data* flow, IpfixRecord::FieldInfo* deltaFi);
	void mapReverseElement(uint32_t tid);
	void reverseFlowBucket(Bucket* bucket);
};

#endif /*FLOWHASHTABLE_H_*/
