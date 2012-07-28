/*
 * NetflowV9 to IPFIX Converter Module
 * Copyright (C) 2009 Gerhard Muenz
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

#include "NetflowV9Converter.hpp"

InstanceManager<IpfixDataRecord> NetflowV9Converter::dataRecordIM("NetflowConverterIpfixDataRecord", 0);
InstanceManager<IpfixTemplateRecord> NetflowV9Converter::templateRecordIM("NetflowConverterIpfixTemplateRecord", 0);
InstanceManager<IpfixTemplateDestructionRecord> NetflowV9Converter::templateDestructionRecordIM("NetflowConverterIpfixTemplateDestructionRecord", 0);

NetflowV9Converter::NetflowV9Converter(bool mode, bool keepsysuptime)
	: copyMode(mode), keepFlowSysUpTime(keepsysuptime) 
{
	if (keepFlowSysUpTime && !copyMode) {
		THROWEXCEPTION("Cannot instantiate NetflowV9Converter that tries to keep the SysUpTime values and does not copy packets");
	}
}
void NetflowV9Converter::onTemplate(IpfixTemplateRecord* record)
{
	boost::shared_ptr<TemplateInfo> templateInfo = record->templateInfo;
	if (templateInfo->setId == TemplateInfo::NetflowTemplate) {
	//if (true) {
		// This should be a new Template for us
		if(uniqueIdToConvInfo.find(templateInfo->getUniqueId()) != uniqueIdToConvInfo.end()) {
			msg(MSG_ERROR, "NetflowV9Converter: Received known Template (id=%u) again, which should not happen.", templateInfo->templateId);
			record->removeReference();
			return;
		}
		 
		// Generate new Template, starting with a copy of Netflow Template
		boost::shared_ptr<TemplateInfo> newTemplateInfo(new TemplateInfo(*templateInfo.get()));

		// Make unique
		newTemplateInfo->setUniqueId();
		// Change Template as needed
		newTemplateInfo->setId = TemplateInfo::IpfixTemplate;

		// Generate conversion info
		ConvInfo myConvInfo;
		myConvInfo.additionalDataLength = 0;
		myConvInfo.templateInfo = newTemplateInfo;
		// sysUpTime: Time in milliseconds since this device was first booted.
		// UNIX Secs (=exportTime): Time in seconds since 0000 UTC 1970, at which the Export Packet leaves the Exporter.
		// ==> Unix seconds when system went up = exportTime - sysUpTime/1000 
		myConvInfo.sysUpUnixSeconds = record->sourceID->exportTime - (record->sourceID->sysUpTime / 1000);
		
		// Look for time stamps which to be converted
		TemplateInfo::FieldInfo* fi;
		for (int i = 0; i < newTemplateInfo->fieldCount; i++) {
			fi = &newTemplateInfo->fieldInfo[i];
			if (fi->type.enterprise == 0) {
				if (fi->type.id == IPFIX_TYPEID_flowStartSysUpTime) {
					// length should be 4 octets
					if(fi->type.length == 4) {
						if (keepFlowSysUpTime) {
							// create new field which should contain flowStartSeconds
							TemplateInfo::FieldInfo newField;
							memcpy(&newField, fi, sizeof(TemplateInfo::FieldInfo));

							newField.type.id = IPFIX_TYPEID_flowStartSeconds;
							newField.offset  = newTemplateInfo->fieldInfo[newTemplateInfo->fieldCount - 1].offset + myConvInfo.additionalDataLength;
							
							myConvInfo.newFields.push_back(newField);
							myConvInfo.additionalDataLength += 4;
						} else {
							// Change Template field
							fi->type.id = IPFIX_TYPEID_flowStartSeconds;
						}
						// Save field index for future reference
						myConvInfo.fieldIndexes.push_back(i);
					} else
						msg(MSG_ERROR, "NetflowV9Converter: flowStartSysUpTime has expected length 4, got %u", fi->type.length);
				} else if (fi->type.id == IPFIX_TYPEID_flowEndSysUpTime) {
					// length should be 4 octets
					if(fi->type.length == 4) {
						if (keepFlowSysUpTime) {
							// create new field which should contain flowStartSeconds
							TemplateInfo::FieldInfo newField;
							memcpy(&newField, fi, sizeof(TemplateInfo::FieldInfo));

							newField.type.id = IPFIX_TYPEID_flowEndSeconds;
							newField.offset  = newTemplateInfo->fieldInfo[newTemplateInfo->fieldCount - 1].offset + myConvInfo.additionalDataLength;

							myConvInfo.newFields.push_back(newField);
							myConvInfo.additionalDataLength += 4;
						} else {
							// Change Template field
							fi->type.id = IPFIX_TYPEID_flowEndSeconds;
						} 
						// Save field index
						myConvInfo.fieldIndexes.push_back(i);
					} else
						msg(MSG_ERROR, "NetflowV9Converter: flowStartSysUpTime has expected length 4, got %u", fi->type.length);
				}
			} else {
				msg(MSG_ERROR, "NetflowV9Converter: Got enterprise specific IE (id=%u, enterprise=%lu, length=%u) in Netflow Template, which should not happen", fi->type.id, fi->type.enterprise, fi->type.length);
			}
		}

		// check if we have to append new fields
		if (myConvInfo.newFields.size() > 0) {
			// ok, append the new fields to the data record
			newTemplateInfo->fieldInfo = (TemplateInfo::FieldInfo*)realloc(newTemplateInfo->fieldInfo, (newTemplateInfo->fieldCount + myConvInfo.newFields.size()) * sizeof(TemplateInfo::FieldInfo));
			if (newTemplateInfo->fieldInfo == NULL) {
				THROWEXCEPTION("Could not allocate memory of new fields!");
			}

			// append the new fields to TemplateInfo
			for (std::list<TemplateInfo::FieldInfo>::const_iterator i = myConvInfo.newFields.begin(); i != myConvInfo.newFields.end(); ++i) {
				memcpy(newTemplateInfo->fieldInfo + newTemplateInfo->fieldCount, &*i, sizeof(TemplateInfo::FieldInfo));
				newTemplateInfo->fieldCount++;
			}

		}

		// Save conversion info in map
		uniqueIdToConvInfo[templateInfo->getUniqueId()] = myConvInfo;

		// Generate and send new Template record
		IpfixTemplateRecord* newTemplateRecord = templateRecordIM.getNewInstance();
		newTemplateRecord->sourceID = record->sourceID;
		newTemplateRecord->templateInfo = newTemplateInfo;
		send(newTemplateRecord);

		// Release original Template record
		record->removeReference();
	} else {
		send(record);
	}
}

void NetflowV9Converter::onTemplateDestruction(IpfixTemplateDestructionRecord* record)
{
	boost::shared_ptr<TemplateInfo> templateInfo = record->templateInfo;
	if (templateInfo->setId == TemplateInfo::NetflowTemplate) {
	//if (true) {
		// This should be a known Template for us
		map<uint16_t, ConvInfo>::iterator iter = uniqueIdToConvInfo.find(templateInfo->getUniqueId());
		if(iter == uniqueIdToConvInfo.end()) {
			msg(MSG_ERROR, "NetflowV9Converter: Received unknown Template (id=%u), which should not happen.", templateInfo->templateId);
			record->removeReference();
			return;
		}
		 
		// Generate and send Template destruction record
		IpfixTemplateDestructionRecord* newTemplateDestructionRecord = templateDestructionRecordIM.getNewInstance();
		newTemplateDestructionRecord->sourceID = record->sourceID;
		newTemplateDestructionRecord->templateInfo = iter->second.templateInfo;
		send(newTemplateDestructionRecord);

		// Delete conversion info from map
		uniqueIdToConvInfo.erase(iter);

		// Release original Template record
		record->removeReference();
	} else {
		send(record);
	}
}

void NetflowV9Converter::onDataRecord(IpfixDataRecord* record)
{
	boost::shared_ptr<TemplateInfo> templateInfo = record->templateInfo;
	if (templateInfo->setId == TemplateInfo::NetflowTemplate) {
	//if (true) {
		// This should be a known Template for us
		map<uint16_t, ConvInfo>::iterator iter = uniqueIdToConvInfo.find(templateInfo->getUniqueId());
		if(iter == uniqueIdToConvInfo.end()) {
			msg(MSG_ERROR, "NetflowV9Converter: Received Data Record associated to unknown Template (id=%u), which should not happen.", templateInfo->templateId);
			record->removeReference();
			return;
		}

		// Copy the record if desired
		IpfixDataRecord* myRecord;
		if(copyMode) {
			// generate copy of the current record
			myRecord = dataRecordIM.getNewInstance();
			myRecord->sourceID = record->sourceID;
			myRecord->dataLength = record->dataLength; // = recordLength
			myRecord->message = boost::shared_array<IpfixRecord::Data>(new IpfixRecord::Data[record->dataLength + iter->second.additionalDataLength]);
			memcpy(myRecord->message.get(), record->data, record->dataLength);
			myRecord->data = myRecord->message.get();
			// release record
			record->removeReference();
		} else
			myRecord = record;
		
		// Change Template
		myRecord->templateInfo = iter->second.templateInfo;

		if (keepFlowSysUpTime) {
			// Convert time stamps and store them into the new fields
			std::list<TemplateInfo::FieldInfo>::const_iterator newField = iter->second.newFields.begin();
			for (std::list<uint16_t>::iterator i = iter->second.fieldIndexes.begin(); i != iter->second.fieldIndexes.end(); i++) {
				uint32_t* oldTimestamp = (uint32_t*) (myRecord->data + templateInfo->fieldInfo[*i].offset);
				uint32_t* newTimestamp = (uint32_t*) (myRecord->data + newField->offset);
				*newTimestamp = htonl(ntohl(*oldTimestamp)/1000 + iter->second.sysUpUnixSeconds);
				newField++;
			}
		} else {
			// Convert time stamps in place
			for (std::list<uint16_t>::iterator i = iter->second.fieldIndexes.begin(); i != iter->second.fieldIndexes.end(); i++) {
				assert(*i < templateInfo->fieldCount);
				uint32_t* timestamp = (uint32_t*) (myRecord->data + templateInfo->fieldInfo[*i].offset);
				*timestamp = htonl(ntohl(*timestamp)/1000 + iter->second.sysUpUnixSeconds);
			}
		}
		// send converted record
		send(myRecord);
	} else {
		send(record);
	}
}

void NetflowV9Converter::onReconfiguration2()
{
	// we do not need to destroy templates during reconfiguration
	// forget all conversion info
	uniqueIdToConvInfo.clear();
}
