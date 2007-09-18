#ifndef FLOWHASHTABLE_H_
#define FLOWHASHTABLE_H_


#include "BaseHashtable.h"

#include <boost/smart_ptr.hpp>

class FlowHashtable : public BaseHashtable
{
public:
	FlowHashtable(Source<IpfixRecord*>* recordsource, Rule* rule, 
			uint16_t minBufferTime, uint16_t maxBufferTime);
	virtual ~FlowHashtable();
	
	void aggregateTemplateData(IpfixRecord::TemplateInfo* ti, IpfixRecord::Data* data);
	void aggregateDataTemplateData(IpfixRecord::DataTemplateInfo* ti, IpfixRecord::Data* data);

private:
	int aggregateField(IpfixRecord::FieldInfo::Type* type, IpfixRecord::Data* baseData, 
			IpfixRecord::Data* deltaData);
	int aggregateFlow(IpfixRecord::Data* baseFlow, IpfixRecord::Data* flow);
	uint16_t getHash(IpfixRecord::Data* data);	
	int equalFlow(IpfixRecord::Data* flow1, IpfixRecord::Data* flow2);
	void bufferDataBlock(boost::shared_array<IpfixRecord::Data> data);
	int equalRaw(IpfixRecord::FieldInfo::Type* data1Type, IpfixRecord::Data* data1, 
			IpfixRecord::FieldInfo::Type* data2Type, IpfixRecord::Data* data2);
	void copyData(IpfixRecord::FieldInfo::Type* dstType, IpfixRecord::Data* dstData, 
			IpfixRecord::FieldInfo::Type* srcType, IpfixRecord::Data* srcData, 
			Rule::Field::Modifier modifier);
	void createMaskedField(IpfixRecord::Data* address, uint8_t imask);
};

#endif /*FLOWHASHTABLE_H_*/
