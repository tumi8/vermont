#ifndef FLOWHASHTABLE_H_
#define FLOWHASHTABLE_H_


#include "BaseHashtable.h"

#include <boost/smart_ptr.hpp>
#include <map>

class FlowHashtable : public BaseHashtable
{
public:
	FlowHashtable(Source<IpfixRecord*>* recordsource, Rule* rule,
			uint16_t minBufferTime, uint16_t maxBufferTime, uint8_t hashbits);
	virtual ~FlowHashtable();

	void aggregateTemplateData(TemplateInfo* ti, IpfixRecord::Data* data);
	void aggregateDataTemplateData(TemplateInfo* ti, IpfixRecord::Data* data);


private:
	uint32_t* revDataTemplateMapper; /**< contains indizes to dataTemplate for a reverse flow*/
	bool biflowAggregation; /**< set to true if biflow aggregation is to be done*/


	/**
	 * contains indizes to reversed flow fields
	 * e.g. srcIP index points to dstIP index, and dstIP index points to srcIP index
	 */
	vector<uint32_t> flowReverseMapper;
	char* switchArray; /**< used by function reverseFlowBucket as temporary storage */

	int aggregateField(TemplateInfo::FieldInfo* basefi, TemplateInfo::FieldInfo* deltafi, IpfixRecord::Data* base,
		  IpfixRecord::Data* delta);
	int aggregateFlow(IpfixRecord::Data* baseFlow, IpfixRecord::Data* flow, bool reverse);
	uint32_t getHash(IpfixRecord::Data* data, bool reverse);
	int equalFlow(IpfixRecord::Data* flow1, IpfixRecord::Data* flow2, bool reverse);
	HashtableBucket* lookupBucket(uint32_t hash, IpfixRecord::Data* data, bool reverse, HashtableBucket** prevbucket);
	void bufferDataBlock(boost::shared_array<IpfixRecord::Data> data);
	int equalRaw(InformationElement::IeInfo* data1Type, IpfixRecord::Data* data1,
			InformationElement::IeInfo* data2Type, IpfixRecord::Data* data2);
	void copyData(TemplateInfo::FieldInfo* dstFI, IpfixRecord::Data* dst,
			TemplateInfo::FieldInfo* srcFI, IpfixRecord::Data* src, Rule::Field::Modifier modifier);
	void genBiflowStructs();
	int compare4ByteField(IpfixRecord::Data* baseFlow, TemplateInfo::FieldInfo* baseFi,
			IpfixRecord::Data* flow, TemplateInfo::FieldInfo* deltaFi);
	int compare8ByteField(IpfixRecord::Data* baseFlow, TemplateInfo::FieldInfo* baseFi,
			IpfixRecord::Data* flow, TemplateInfo::FieldInfo* deltaFi);
	void mapReverseElement(uint32_t tid);
	void reverseFlowBucket(HashtableBucket* bucket);
};

#endif /*FLOWHASHTABLE_H_*/
