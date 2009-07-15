#include "IpfixRecordAnonymizer.h"

InstanceManager<IpfixDataRecord> IpfixRecordAnonymizer::dataRecordIM("IpfixDataRecord");

void IpfixRecordAnonymizer::setCopyMode(bool mode)
{
	copyMode = mode;
}

void IpfixRecordAnonymizer::onTemplate(IpfixTemplateRecord* record)
{
	send(record);
}

void IpfixRecordAnonymizer::onOptionsTemplate(IpfixOptionsTemplateRecord* record)
{
	send(record);
}

void IpfixRecordAnonymizer::onOptionsRecord(IpfixOptionsRecord* record)
{
	send(record);
}

void IpfixRecordAnonymizer::onDataRecord(IpfixDataRecord* record)
{
	IpfixDataRecord* myRecord;
	if(copyMode) {
		// generate copy of the current record
		myRecord = dataRecordIM.getNewInstance();
		myRecord->sourceID = record->sourceID;
		// we also need to copy the Data Template Info
		//myRecord->templateInfo = boost::shared_ptr<IpfixRecord::DataTemplateInfo>(new IpfixRecord::DataTemplateInfo(*record->dataTemplateInfo.get()));
		myRecord->templateInfo = record->templateInfo;
		myRecord->dataLength = record->dataLength; // = recordLength
		myRecord->message = boost::shared_array<IpfixRecord::Data>(new IpfixRecord::Data[record->dataLength]);
		memcpy(myRecord->message.get(), record->data, record->dataLength);
		myRecord->data = myRecord->message.get();
		// release record
		record->removeReference();
	} else
		myRecord = record;

	// anonymize data template fixed value fields if necessary
	if(!myRecord->templateInfo->anonymized) {
		for (int i = 0; i != myRecord->templateInfo->dataCount; ++i) {
			IpfixRecord::FieldInfo* field = myRecord->templateInfo->dataInfo + i;
			anonField(field->type.id, myRecord->templateInfo->data + field->offset, field->type.length);
		}
		myRecord->templateInfo->anonymized = true; 
	}
	// anonymize variable value fields
	for (int i = 0; i != myRecord->templateInfo->fieldCount; ++i) {
		IpfixRecord::FieldInfo* field = myRecord->templateInfo->fieldInfo + i;
		anonField(field->type.id, myRecord->data + field->offset, field->type.length);
	}
	send(myRecord);
}

void IpfixRecordAnonymizer::onTemplateDestruction(IpfixTemplateDestructionRecord* record)
{
	send(record);
}

void IpfixRecordAnonymizer::onOptionsTemplateDestruction(IpfixOptionsTemplateDestructionRecord* record)
{
	send(record);
}

