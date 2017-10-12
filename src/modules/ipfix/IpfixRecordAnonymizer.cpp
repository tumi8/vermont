/*
 * IPFIX Record Anonymizer
 * Copyright (C) 2008 Lothar Braun
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

#include "IpfixRecordAnonymizer.h"

InstanceManager<IpfixDataRecord> IpfixRecordAnonymizer::dataRecordIM("RecordAnonymizerIpfixDataRecord", 0);

void IpfixRecordAnonymizer::setCopyMode(bool mode)
{
	copyMode = mode;
}

void IpfixRecordAnonymizer::onTemplate(IpfixTemplateRecord* record)
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
		myRecord->templateInfo = record->templateInfo;
		myRecord->dataLength = record->dataLength; // = recordLength
		myRecord->message = boost::shared_array<IpfixRecord::Data>(new IpfixRecord::Data[record->dataLength]);
		memcpy(myRecord->message.get(), record->data, record->dataLength);
		myRecord->data = myRecord->message.get();
		// release record
		record->removeReference();
	} else
		myRecord = record;

	boost::shared_ptr<TemplateInfo> templateInfo = myRecord->templateInfo;

	// anonymize Data Record fields
	int32_t lastanonid = -2;
	for (int i = 0; i != templateInfo->fieldCount; ++i) {
		TemplateInfo::FieldInfo* field = templateInfo->fieldInfo + i;
		if (lastanonid==i-1
				&& field->type==InformationElement::IeInfo(IPFIX_ETYPEID_anonymisationType, IPFIX_PEN_vermont)) {
			// only if the preceding information element was really anonymised, we set this IE to 1
			*(uint8_t*)(myRecord->data+field->offset) = 1;
		} else if (anonField(field->type, myRecord->data + field->offset, field->type.length)) {
			lastanonid = i;
		}
	}

	// anonymize scope fields
	if((record->templateInfo->scopeCount != 0) && ((record->templateInfo->setId == TemplateInfo::IpfixOptionsTemplate)
			|| (record->templateInfo->setId == TemplateInfo::NetflowOptionsTemplate))) {
		for (int i = 0; i != templateInfo->scopeCount; ++i) {
			TemplateInfo::FieldInfo* field = templateInfo->scopeInfo + i;
			anonField(field->type, myRecord->data + field->offset, field->type.length);
		}
	}

	send(myRecord);
}


void IpfixRecordAnonymizer::onTemplateDestruction(IpfixTemplateDestructionRecord* record)
{
	send(record);
}


