#include "IpfixRecordAnonymizer.h"

InstanceManager<IpfixDataRecord> IpfixRecordAnonymizer::dataRecordIM("IpfixDataRecord");

void IpfixRecordAnonymizer::setCopyMode(bool mode)
{
	copyMode = mode;
}

void IpfixRecordAnonymizer::onTemplate(IpfixTemplateRecord* record)
{
	//TODO: anonymize Data Templates
	if((record->templateInfo->setId == TemplateInfo::IpfixDataTemplate) && (record->templateInfo->dataCount != 0)) {
		for (int i = 0; i != record->templateInfo->dataCount; ++i) {
			TemplateInfo::FieldInfo* field = record->templateInfo->dataInfo + i;
			// check if this fixed value field should be anonymized
			if (methods.find(field->type.id) != methods.end())
				msg(MSG_ERROR, "IpfixRecordAnonymizer: Anonymization not supported for fixed value field (ie=%u, enterprise=%u) in Data Template (id=%u)", field->type.id, field->type.enterprise, record->templateInfo->templateId);
				
		}
	}
	send(record);
}

void IpfixRecordAnonymizer::onDataRecord(IpfixDataRecord* record)
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

	/* TODO (Gerhard 12/2009): Anonymization of Data Template does not make sense if implemented at this place (only). 
	 * For example, the IpfixSender uses the Templates received by onTemplate(...), not the ones linked to the
	 * Data Records. Therefore, anonymization should take place in IpfixRecordAnonymizer::onTemplate().
	 * The anonymized Data Template should then be linked to all the corresponding Data Records.
	 *
	// anonymize data template fixed value fields if necessary
	if((myRecord->templateInfo->setId==TemplateInfo::IpfixDataRecord) && (!myRecord->templateInfo->anonymized)) {
		// copy Data Template Info in copy mode
		if(copyMode)
			myRecord->templateInfo = boost::shared_ptr<TemplateInfo>(new TemplateInfo(*record->templateInfo.get()));

		for (int i = 0; i != myRecord->templateInfo->dataCount; ++i) {
			TemplateInfo::FieldInfo* field = myRecord->templateInfo->dataInfo + i;
			anonField(field->type.id, myRecord->templateInfo->data + field->offset, field->type.length);
		}
		myRecord->templateInfo->anonymized = true; 
	}
	*/

	boost::shared_ptr<TemplateInfo> templateInfo = myRecord->templateInfo;

	//TODO: enterprise number should be considered (Gerhard 12/2009)
	
	// anonymize Data Record fields
	for (int i = 0; i != templateInfo->fieldCount; ++i) {
		TemplateInfo::FieldInfo* field = templateInfo->fieldInfo + i;
		anonField(field->type.id, myRecord->data + field->offset, field->type.length);
	}

	// anonymize scope fields
	if((record->templateInfo->scopeCount != 0) && ((record->templateInfo->setId == TemplateInfo::IpfixOptionsTemplate) || (record->templateInfo->setId == TemplateInfo::NetflowOptionsTemplate))) {
		for (int i = 0; i != templateInfo->scopeCount; ++i) {
			TemplateInfo::FieldInfo* field = templateInfo->scopeInfo + i;
			anonField(field->type.id, myRecord->data + field->offset, field->type.length);
		}
	}

	send(myRecord);
}


void IpfixRecordAnonymizer::onTemplateDestruction(IpfixTemplateDestructionRecord* record)
{
	send(record);
}


