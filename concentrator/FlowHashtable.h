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
	
	int aggregateField(IpfixRecord::FieldInfo::Type* type, IpfixRecord::Data* baseData, 
			IpfixRecord::Data* deltaData);
	int aggregateFlow(IpfixRecord::Data* baseFlow, IpfixRecord::Data* flow, bool reverse);
	uint16_t getHash(IpfixRecord::Data* data, bool reverse);	
	int equalFlow(IpfixRecord::Data* flow1, IpfixRecord::Data* flow2, bool reverse);
	Bucket* lookupBucket(uint32_t hash, IpfixRecord::Data* data, bool reverse);
	void bufferDataBlock(boost::shared_array<IpfixRecord::Data> data);
	int equalRaw(IpfixRecord::FieldInfo::Type* data1Type, IpfixRecord::Data* data1, 
			IpfixRecord::FieldInfo::Type* data2Type, IpfixRecord::Data* data2);
	void copyData(IpfixRecord::FieldInfo::Type* dstType, IpfixRecord::Data* dstData, 
			IpfixRecord::FieldInfo::Type* srcType, IpfixRecord::Data* srcData, 
			Rule::Field::Modifier modifier);
	void genBiflowStructs();
};

#endif /*FLOWHASHTABLE_H_*/
