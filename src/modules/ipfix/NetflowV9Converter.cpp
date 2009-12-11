#include "NetflowV9Converter.hpp"

InstanceManager<IpfixDataRecord> NetflowV9Converter::dataRecordIM("nfConvIpfixDataRecord");
InstanceManager<IpfixTemplateRecord> NetflowV9Converter::templateRecordIM("nvConvIpfixTemplateRecord");
InstanceManager<IpfixTemplateDestructionRecord> NetflowV9Converter::templateDestructionRecordIM("nvConvIpfixTemplateDestructionRecord");

void NetflowV9Converter::setCopyMode(bool mode)
{
	copyMode = mode;
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
						// Change Template field
						fi->type.id = IPFIX_TYPEID_flowStartSeconds;
						// Save field index
						myConvInfo.fieldIndexes.push_back(i);
					} else
						msg(MSG_ERROR, "NetflowV9Converter: flowStartSysUpTime has expected length 4, got %u", fi->type.length);
				} else if (fi->type.id == IPFIX_TYPEID_flowEndSysUpTime) {
					// length should be 4 octets
					if(fi->type.length == 4) {
						// Change Template field
						fi->type.id = IPFIX_TYPEID_flowEndSeconds;
						// Save field index
						myConvInfo.fieldIndexes.push_back(i);
					} else
						msg(MSG_ERROR, "NetflowV9Converter: flowStartSysUpTime has expected length 4, got %u", fi->type.length);
				}
				/*
				// TODO: remove this test code:	
				else if (fi->type.id == IPFIX_TYPEID_flowStartMilliSeconds)
				{
					fi->type.id = IPFIX_TYPEID_flowStartSeconds;
					fi->type.length = 4;
				        myConvInfo.fieldIndexes.push_back(i);
				}
				*/
			} else {
				msg(MSG_ERROR, "NetflowV9Converter: Got enterprise specific IE (id=%u, enterprise=%lu, length=%u) in Netflow Template, which should not happen", fi->type.id, fi->type.enterprise, fi->type.length);
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
			myRecord->message = boost::shared_array<IpfixRecord::Data>(new IpfixRecord::Data[record->dataLength]);
			memcpy(myRecord->message.get(), record->data, record->dataLength);
			myRecord->data = myRecord->message.get();
			// release record
			record->removeReference();
		} else
			myRecord = record;
		
		// Change Template
		myRecord->templateInfo = iter->second.templateInfo;

		// Convert time stamps
		for (std::list<uint16_t>::iterator i = iter->second.fieldIndexes.begin(); i != iter->second.fieldIndexes.end(); i++) {
			assert(*i < templateInfo->fieldCount);
			uint32_t* timestamp = (uint32_t*) (myRecord->data + templateInfo->fieldInfo[*i].offset);
			*timestamp = htonl(ntohl(*timestamp)/1000 + iter->second.sysUpUnixSeconds);
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
