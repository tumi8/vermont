#include "RecordAnonymizer.h"

InstanceManager<IpfixDataRecord> RecordAnonymizer::dataRecordIM("IpfixDataRecord");
InstanceManager<IpfixDataDataRecord> RecordAnonymizer::dataDataRecordIM("IpfixDataDataRecord");

void RecordAnonymizer::setCopyMode(bool mode)
{
	copyMode = mode;
}

void RecordAnonymizer::onTemplate(IpfixTemplateRecord* record)
{
	send(record);
}

void RecordAnonymizer::onOptionsTemplate(IpfixOptionsTemplateRecord* record)
{
	send(record);
}


void RecordAnonymizer::onDataTemplate(IpfixDataTemplateRecord* record)
{
	send(record);
}

void RecordAnonymizer::onDataRecord(IpfixDataRecord* record)
{
	IpfixDataRecord* myRecord;
	if(copyMode) {
		// generate copy of the current record
		myRecord = dataRecordIM.getNewInstance();
		myRecord->sourceID = record->sourceID;
		myRecord->templateInfo = record->templateInfo;
		myRecord->dataLength = record->dataLength; // = recordLength
		myRecord->message = boost::shared_array<IpfixRecord::Data>(new IpfixRecord::Data[record->dataLength]);
		memcpy(myRecord->message.get(), record->data, record->dataLength);
		myRecord->data = myRecord->message.get();
		// release record
		record->removeReference();
	} else
		myRecord = record;

	for (int i = 0; i != myRecord->templateInfo->fieldCount; ++i) {
		IpfixRecord::FieldInfo* field = myRecord->templateInfo->fieldInfo + i;
		anonField(field->type.id, myRecord->data + field->offset, field->type.length);
	}
	send(myRecord);
}


void RecordAnonymizer::onOptionsRecord(IpfixOptionsRecord* record)
{
	send(record);
}

void RecordAnonymizer::onDataDataRecord(IpfixDataDataRecord* record)
{
	IpfixDataDataRecord* myRecord;
	if(copyMode) {
		// generate copy of the current record
		myRecord = dataDataRecordIM.getNewInstance();
		myRecord->sourceID = record->sourceID;
		// we also need to copy the Data Template Info
		myRecord->dataTemplateInfo = boost::shared_ptr<IpfixRecord::DataTemplateInfo>(new IpfixRecord::DataTemplateInfo(*record->dataTemplateInfo.get()));
		//myRecord->dataTemplateInfo = record->dataTemplateInfo;
		myRecord->dataLength = record->dataLength; // = recordLength
		myRecord->message = boost::shared_array<IpfixRecord::Data>(new IpfixRecord::Data[record->dataLength]);
		memcpy(myRecord->message.get(), record->data, record->dataLength);
		myRecord->data = myRecord->message.get();
		// release record
		record->removeReference();
	} else
		myRecord = record;

	// anonymize data template fixed value fields if necessary
	if(!myRecord->dataTemplateInfo->anonymized) {
		for (int i = 0; i != myRecord->dataTemplateInfo->dataCount; ++i) {
			IpfixRecord::FieldInfo* field = myRecord->dataTemplateInfo->dataInfo + i;
			anonField(field->type.id, myRecord->dataTemplateInfo->data + field->offset, field->type.length);
		}
		myRecord->dataTemplateInfo->anonymized = true; 
	}
	// anonymize variable value fields
	for (int i = 0; i != myRecord->dataTemplateInfo->fieldCount; ++i) {
		IpfixRecord::FieldInfo* field = myRecord->dataTemplateInfo->fieldInfo + i;
		anonField(field->type.id, myRecord->data + field->offset, field->type.length);
	}
	send(myRecord);
}

void RecordAnonymizer::onTemplateDestruction(IpfixTemplateDestructionRecord* record)
{
	send(record);
}

void RecordAnonymizer::onOptionsTemplateDestruction(IpfixOptionsTemplateDestructionRecord* record)
{
	send(record);
}


void RecordAnonymizer::onDataTemplateDestruction(IpfixDataTemplateDestructionRecord* record)
{
	send(record);
}


