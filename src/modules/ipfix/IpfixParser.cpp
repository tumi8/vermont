/*
 * IPFIX Concentrator Module Library
 * Copyright (C) 2004 Christoph Sommer <http://www.deltadevelopment.de/users/christoph/ipfix/>
 *               2009 Gerhard Muenz
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


#include <sstream>

/* for ntohll et al */
#include "common/ipfixlolib/ipfixlolib.h"

#include "IpfixParser.hpp"
#include "TemplateBuffer.hpp"
#include "common/ipfixlolib/ipfix.h"
//#include "IpfixPrinter.hpp"

#include "common/msg.h"

using namespace std;

#define MAX_MSG_LEN 65536

//static variables
InstanceManager<IpfixTemplateRecord> IpfixParser::templateRecordIM("ParserIpfixTemplateRecord", 0);
InstanceManager<IpfixDataRecord> IpfixParser::dataRecordIM("ParserIpfixDataRecord", 0);
InstanceManager<IpfixTemplateDestructionRecord> IpfixParser::templateDestructionRecordIM("ParserIpfixTemplateDestructionRecord", 0);


/**
 * Processes an IPFIX template set.
 * Called by processMessage
 * ATTENTION: setId needs to be TemplateInfo::IpfixTemplate or TemplateInfo::NetflowTemplate
 */
uint32_t IpfixParser::processTemplateSet(boost::shared_ptr<IpfixRecord::SourceID> sourceId, TemplateInfo::SetId setId, boost::shared_array<uint8_t> message, IpfixSetHeader* set, uint8_t* endOfMessage) {
	uint32_t numberOfRecords = 0;
	uint8_t* endOfSet = (uint8_t*)set + ntohs(set->length);
	uint8_t* record = (uint8_t*)&set->data;

	/* check if set length lies within message boundaries */
	if (endOfSet > endOfMessage) {
		msg(MSG_ERROR, "IpfixParser: Template set exceeds message boundary!");
		return 0;
	}

	/* Template records are >= 4 byte, so we stop processing when fewer bytes are left */
	while (record + 4 <= endOfSet) {
		IpfixTemplateHeader* th = (IpfixTemplateHeader*)record;
		record = (uint8_t*)&th->data;
		if (th->fieldCount == 0) {
			/* This is a Template withdrawal message */
			templateBuffer->destroyBufferedTemplate(sourceId, ntohs(th->templateId));
			numberOfRecords++;
			continue;
		}
		TemplateBuffer::BufferedTemplate* bt = new TemplateBuffer::BufferedTemplate;
		boost::shared_ptr<TemplateInfo> ti(new TemplateInfo);
		bt->sourceID = sourceId;
		bt->recordLength = 0;
		bt->templateInfo = ti;
		ti->templateId = ntohs(th->templateId);
		ti->setId = setId;
		ti->fieldCount = ntohs(th->fieldCount);
		ti->fieldInfo = TemplateInfo::allocateFieldInfoArray(ti->fieldCount);
		int isLengthVarying = 0;
		uint16_t fieldNo;
		for (fieldNo = 0; fieldNo < ti->fieldCount; fieldNo++) {
			/* check if there are at least 4 bytes for this field */
			if (record+4 > endOfSet) {
				msg(MSG_ERROR, "IpfixParser: Template record (id=%u) exceeds set boundary!", bt->templateInfo->templateId);
				delete bt;
				return numberOfRecords;
			}
			ti->fieldInfo[fieldNo].type.id = ntohs(*(uint16_t*)((uint8_t*)record+0));
			ti->fieldInfo[fieldNo].type.length = ntohs(*(uint16_t*)((uint8_t*)record+2));
			ti->fieldInfo[fieldNo].isVariableLength = (ti->fieldInfo[fieldNo].type.length == 65535);
			ti->fieldInfo[fieldNo].offset = bt->recordLength; 
			bt->recordLength+=ti->fieldInfo[fieldNo].type.length;
			if (ti->fieldInfo[fieldNo].type.length == 65535) {
				isLengthVarying=1;
			}
			// IPFIX only: If hightest bit of field id is set (0x8000), we must look for an enterprise number.
			if ((ti->fieldInfo[fieldNo].type.id & IPFIX_ENTERPRISE_TYPE) && setId == TemplateInfo::IpfixTemplate) {
				/* check if there are 8 bytes for this field */
				if (record+8 > endOfSet) {
					msg(MSG_ERROR, "IpfixParser: Template record (id=%u) exceeds set boundary!", bt->templateInfo->templateId);
					delete bt;
					return numberOfRecords;
				}
				ti->fieldInfo[fieldNo].type.enterprise = ntohl(*(uint32_t*)((uint8_t*)record+4));
				// remove highest bit
				ti->fieldInfo[fieldNo].type.id &= ~IPFIX_ENTERPRISE_TYPE;
				record = (uint8_t*)((uint8_t*)record+8);
			} else {
				ti->fieldInfo[fieldNo].type.enterprise = 0;
				record = (uint8_t*)((uint8_t*)record+4);
			}
		}
		if (isLengthVarying) {
			bt->recordLength = 65535;
			for (fieldNo = 0; fieldNo < ti->fieldCount; fieldNo++) {
				ti->fieldInfo[fieldNo].offset = 0xFFFFFFFF;
			}
		}
        
		templateBuffer->bufferTemplate(bt); 
		if((sourceId->protocol == IPFIX_protocolIdentifier_UDP) && (templateLifetime > 0))
			bt->expires = time(0) + templateLifetime;
		else
			bt->expires = 0;

		IpfixTemplateRecord* ipfixRecord = templateRecordIM.getNewInstance();
		ipfixRecord->sourceID = sourceId;
		ipfixRecord->templateInfo = ti;
		push(ipfixRecord);
		numberOfRecords++;
	}
	return numberOfRecords;
}

/**
 * Processes an IPFIX Options Template Set.
 * Called by processMessage
 * returns number of processed records
 * ATTENTION: setId needs to be TemplateInfo::IpfixOptionsTemplate or TemplateInfo::NetflowOptionsTemplate
 */
uint32_t IpfixParser::processOptionsTemplateSet(boost::shared_ptr<IpfixRecord::SourceID> sourceId, TemplateInfo::SetId setId, boost::shared_array<uint8_t> message, IpfixSetHeader* set, uint8_t* endOfMessage) {
	uint32_t numberOfRecords = 0;
	uint8_t* endOfSet = (uint8_t*)set + ntohs(set->length);
	uint8_t* record = (uint8_t*)&set->data;

	/* check if set length lies within message boundaries */
	if (endOfSet > endOfMessage) {
		msg(MSG_ERROR, "IpfixParser: Options Template set exceeds message boundary!");
		return 0;
	}

	/* Options template records are >= 4 byte, so we stop processing when fewer are left */
	while (record + 4 <= endOfSet) {
		/* First cast to normal template header used for withdrawal message */
		IpfixTemplateHeader* th = (IpfixTemplateHeader*)record;
		record = (uint8_t*)&th->data;
		if (th->fieldCount == 0) {
			/* This is a Template withdrawal message */
			templateBuffer->destroyBufferedTemplate(sourceId, ntohs(th->templateId));
			numberOfRecords++;
			continue;
		}

		IpfixOptionsTemplateHeader* oth = (IpfixOptionsTemplateHeader*)th;
		record = (uint8_t*)&oth->data;

		/* Non-withdrawal options template records are >= 6 byte */
		if (record > endOfSet) {
			msg(MSG_ERROR, "IpfixParser: Strange long padding in Options Template");
			return numberOfRecords;
		}

		TemplateBuffer::BufferedTemplate* bt = new TemplateBuffer::BufferedTemplate;
		boost::shared_ptr<TemplateInfo> ti(new TemplateInfo);
		bt->sourceID = sourceId;
		bt->recordLength = 0;
		bt->templateInfo = ti;
		ti->templateId = ntohs(oth->templateId);
		ti->setId = setId;
		ti->scopeCount = ntohs(oth->scopeCount);
		ti->scopeInfo = TemplateInfo::allocateFieldInfoArray(ti->scopeCount);
		ti->fieldCount = ntohs(oth->fieldCount)-ntohs(oth->scopeCount);
		ti->fieldInfo = TemplateInfo::allocateFieldInfoArray(ti->fieldCount);
		int isLengthVarying = 0;
		uint16_t scopeNo = 0;
		//for loop works for IPFIX, but in the case of NetflowV9, scopeCount is the length of all fields in bytes
		//for (scopeNo = 0; scopeNo < ti->scopeCount; scopeNo++) {
		while (scopeNo < ti->scopeCount) {
			/* check if there are at least 4 bytes for this field */
			if (record+4 > endOfSet) {
				msg(MSG_ERROR, "IpfixParser: Options Template record exceeds set boundary!");
				delete bt;
				return numberOfRecords;
			}
			ti->scopeInfo[scopeNo].type.id = ntohs(*(uint16_t*)((uint8_t*)record+0));
			ti->scopeInfo[scopeNo].type.length = ntohs(*(uint16_t*)((uint8_t*)record+2));
			ti->scopeInfo[scopeNo].isVariableLength = (ti->scopeInfo[scopeNo].type.length == 65535);
			ti->scopeInfo[scopeNo].offset = bt->recordLength; 
			bt->recordLength+=ti->scopeInfo[scopeNo].type.length;
			if (ti->scopeInfo[scopeNo].type.length == 65535) {
				isLengthVarying=1;
			}
			// IPFIX only: If hightest bit of field id is set (0x8000), we must look for an enterprise number.
			if ((ti->scopeInfo[scopeNo].type.id & IPFIX_ENTERPRISE_TYPE) && setId == TemplateInfo::IpfixOptionsTemplate) {
				/* check if there are 8 bytes for this field */
				if (record+8 > endOfSet) {
					msg(MSG_ERROR, "IpfixParser: Options Template record exceeds set boundary!");
					delete bt;
					return numberOfRecords;
				}
				ti->scopeInfo[scopeNo].type.enterprise = ntohl(*(uint32_t*)((uint8_t*)record+4));
				// remove highest bit
				ti->scopeInfo[scopeNo].type.id &= ~IPFIX_ENTERPRISE_TYPE;
				record = (uint8_t*)((uint8_t*)record+8);
			} else {
				ti->fieldInfo[scopeNo].type.enterprise = 0;
				record = (uint8_t*)((uint8_t*)record+4);
			}
			if (setId == TemplateInfo::NetflowOptionsTemplate) {
				scopeNo += ti->scopeInfo[scopeNo].type.length;
				if (scopeNo > ti->scopeCount) {
					msg(MSG_ERROR, "IpfixParser: Scope fields in Netflow Options Template exceed scope boundary!");
					delete bt;
					return numberOfRecords;
				}
			} else
				scopeNo++;
		}
		uint16_t fieldNo = 0;
		//for loop works for IPFIX, but in the case of NetflowV9, fieldCount is the length of all fields in bytes
		//for (fieldNo = 0; fieldNo < ti->fieldCount; fieldNo++) {
		while (fieldNo < ti->fieldCount) {
			/* check if there are at least 4 bytes for this field */
			if (record+4 > endOfSet) {
				msg(MSG_ERROR, "IpfixParser: Template record exceeds set boundary!");
				delete bt;
				return numberOfRecords;
			}
			ti->fieldInfo[fieldNo].type.id = ntohs(*(uint16_t*)((uint8_t*)record+0));
			ti->fieldInfo[fieldNo].type.length = ntohs(*(uint16_t*)((uint8_t*)record+2));
			ti->fieldInfo[fieldNo].isVariableLength = (ti->fieldInfo[fieldNo].type.length == 65535);
			ti->fieldInfo[fieldNo].offset = bt->recordLength; 
			bt->recordLength+=ti->fieldInfo[fieldNo].type.length;
			if (ti->fieldInfo[fieldNo].type.length == 65535) {
				isLengthVarying=1;
			}
			// IPFIX only: If hightest bit of field id is set (0x8000), we must look for an enterprise number.
			if ((ti->fieldInfo[fieldNo].type.id & IPFIX_ENTERPRISE_TYPE) && setId == TemplateInfo::IpfixOptionsTemplate) {
				/* check if there are 8 bytes for this field */
				if (record+8 > endOfSet) {
					msg(MSG_ERROR, "IpfixParser: Template record exceeds set boundary!");
					delete bt;
					return numberOfRecords;
				}
				ti->fieldInfo[fieldNo].type.enterprise = ntohl(*(uint32_t*)((uint8_t*)record+4));
				// remove highest bit
				ti->fieldInfo[fieldNo].type.id &= ~IPFIX_ENTERPRISE_TYPE;
				record = (uint8_t*)((uint8_t*)record+8);
			} else {
				ti->fieldInfo[fieldNo].type.enterprise = 0;
				record = (uint8_t*)((uint8_t*)record+4);
			}
			if (setId == TemplateInfo::NetflowOptionsTemplate) {
				fieldNo += ti->fieldInfo[fieldNo].type.length;
				if (fieldNo > ti->fieldCount) {
					msg(MSG_ERROR, "IpfixParser: Fields in Netflow Options Template exceed field boundary!");
					delete bt;
					return numberOfRecords;
				}
			} else
				fieldNo++;
		}
		if (isLengthVarying) {
			bt->recordLength = 65535;
			for (fieldNo = 0; fieldNo < ti->scopeCount; fieldNo++) {
				ti->scopeInfo[fieldNo].offset = 0xFFFFFFFF;
			}
			for (fieldNo = 0; fieldNo < ti->fieldCount; fieldNo++) {
				ti->fieldInfo[fieldNo].offset = 0xFFFFFFFF;
			}
		}
		templateBuffer->bufferTemplate(bt); 
		if((sourceId->protocol == IPFIX_protocolIdentifier_UDP) && (templateLifetime > 0))
			bt->expires = time(0) + templateLifetime;
		else
			bt->expires = 0;

		IpfixTemplateRecord* ipfixRecord = templateRecordIM.getNewInstance();
		ipfixRecord->sourceID = sourceId;
		ipfixRecord->templateInfo = ti;
		push(ipfixRecord);
		numberOfRecords++;
	}
	return numberOfRecords;
}

/**
 * Processes an IPFIX DataTemplate set.
 * Called by processMessage
 * returns number of processed records
 */
uint32_t IpfixParser::processDataTemplateSet(boost::shared_ptr<IpfixRecord::SourceID> sourceId, boost::shared_array<uint8_t> message, IpfixSetHeader* set, uint8_t* endOfMessage) {
	uint32_t numberOfRecords = 0;
	uint8_t* endOfSet = (uint8_t*)set + ntohs(set->length);
	uint8_t* record = (uint8_t*)&set->data;

	/* check if set length lies within message boundaries */
	if (endOfSet > endOfMessage) {
		msg(MSG_ERROR, "IpfixParser: Data Template set exceeds message boundary!");
		return 0;
	}

	/* Data template record are >= 4 byte, so we stop processing when fewer bytes are left */
	while (record + 4 <= endOfSet) {
		/* First cast to normal template header used for withdrawal message */
		IpfixTemplateHeader* th = (IpfixTemplateHeader*)record;
		record = (uint8_t*)&th->data;
		if (th->fieldCount == 0) {
			/* This is a Template withdrawal message */
			templateBuffer->destroyBufferedTemplate(sourceId, ntohs(th->templateId));
			numberOfRecords++;
			continue;
		}

		IpfixDataTemplateHeader* dth = (IpfixDataTemplateHeader*)th;
		record = (uint8_t*)&dth->data;
		/* Non-withdrawal data template records are >= 8 byte */
		if (record > endOfSet) {
			msg(MSG_ERROR, "IpfixParser: Strange long padding in Data Template");
			return numberOfRecords;
		}

		TemplateBuffer::BufferedTemplate* bt = new TemplateBuffer::BufferedTemplate;
		boost::shared_ptr<TemplateInfo> ti(new TemplateInfo);
		bt->sourceID = sourceId;
		bt->recordLength = 0;
		bt->templateInfo = ti;
		ti->templateId = ntohs(dth->templateId);
		ti->setId = TemplateInfo::IpfixDataTemplate;
		ti->preceding = ntohs(dth->precedingRule);
		ti->fieldCount = ntohs(dth->fieldCount);
		ti->dataCount = ntohs(dth->dataCount);
		ti->fieldInfo = TemplateInfo::allocateFieldInfoArray(ti->fieldCount);
		int isLengthVarying = 0;
		uint16_t fieldNo;
		for (fieldNo = 0; fieldNo < ti->fieldCount; fieldNo++) {
			/* check if there are at least 4 bytes for this field */
			if (record+4 > endOfSet) {
				msg(MSG_ERROR, "IpfixParser: Data Template record exceeds set boundary!");
				delete bt;
				return numberOfRecords;
			}
			ti->fieldInfo[fieldNo].type.id = ntohs(*(uint16_t*)((uint8_t*)record+0));
			ti->fieldInfo[fieldNo].type.length = ntohs(*(uint16_t*)((uint8_t*)record+2));
			ti->fieldInfo[fieldNo].isVariableLength = (ti->fieldInfo[fieldNo].type.length == 65535);
			ti->fieldInfo[fieldNo].offset = bt->recordLength; 
			bt->recordLength+=ti->fieldInfo[fieldNo].type.length;
			if (ti->fieldInfo[fieldNo].type.length == 65535) {
				isLengthVarying=1;
			}
			if (ti->fieldInfo[fieldNo].type.id & IPFIX_ENTERPRISE_TYPE) {
				/* check if there are 8 bytes for this field */
				if (record+8 > endOfSet) {
					msg(MSG_ERROR, "IpfixParser: Data Template record exceeds set boundary!");
					delete bt;
					return numberOfRecords;
				}
				ti->fieldInfo[fieldNo].type.enterprise = ntohl(*(uint32_t*)((uint8_t*)record+4));
				// remove highest bit
				ti->fieldInfo[fieldNo].type.id &= ~IPFIX_ENTERPRISE_TYPE;
				record = (uint8_t*)((uint8_t*)record+8);
			} else {
				ti->fieldInfo[fieldNo].type.enterprise = 0;
				record = (uint8_t*)((uint8_t*)record+4);
			}
		}
		if (isLengthVarying) {
			bt->recordLength = 65535;
			for (fieldNo = 0; fieldNo < ti->fieldCount; fieldNo++) {
				ti->fieldInfo[fieldNo].offset = 0xFFFFFFFF;
			}
		}

		ti->dataInfo = (TemplateInfo::FieldInfo*)malloc(ti->dataCount * sizeof(TemplateInfo::FieldInfo));
		for (fieldNo = 0; fieldNo < ti->dataCount; fieldNo++) {
			/* check if there are at least 4 bytes for this field */
			if (record+4 > endOfSet) {
				msg(MSG_ERROR, "IpfixParser: Data Template record exceeds set boundary!");
				delete bt;
				return numberOfRecords;
			}
			ti->dataInfo[fieldNo].type.id = ntohs(*(uint16_t*)((uint8_t*)record+0));
			ti->dataInfo[fieldNo].type.length = ntohs(*(uint16_t*)((uint8_t*)record+2));
			if (ti->dataInfo[fieldNo].type.id & IPFIX_ENTERPRISE_TYPE) {
				/* check if there are 8 bytes for this field */
				if (record+8 >= endOfSet) {
					msg(MSG_ERROR, "IpfixParser: Data Template record exceeds set boundary!");
					delete bt;
					return numberOfRecords;
				}
				ti->dataInfo[fieldNo].type.enterprise = ntohl(*(uint32_t*)((uint8_t*)record+4));
				// remove highest bit
				ti->dataInfo[fieldNo].type.id &= ~IPFIX_ENTERPRISE_TYPE;
				record = (uint8_t*)((uint8_t*)record+8);
			} else {
				ti->dataInfo[fieldNo].type.enterprise = 0;
				record = (uint8_t*)((uint8_t*)record+4);
			}
		}

		int dataLength = 0;
		for (fieldNo = 0; fieldNo < ti->dataCount; fieldNo++) {
			if (ti->dataInfo[fieldNo].type.length == 65535) {
				/* check if there is 1 byte for the length */
				if (record + dataLength + 1 > endOfSet) {
					msg(MSG_ERROR, "IpfixParser: Template record exceeds set boundary!");
					delete bt;
					return numberOfRecords;
				}
				/* This is a variable-length field, get length from first byte and advance offset */
				ti->dataInfo[fieldNo].type.length = *(uint8_t*)(record + dataLength);
				dataLength += 1;
				if (ti->dataInfo[fieldNo].type.length == 255) {
					/* check if there are 2 bytes for the length */
					if (record + dataLength + 2 > endOfSet) {
						msg(MSG_ERROR, "IpfixParser: Template record exceeds set boundary!");
						delete bt;
						return numberOfRecords;
					}
					/* First byte did not suffice, length is stored in next two bytes. Advance offset */
					ti->dataInfo[fieldNo].type.length = *(uint16_t*)(record + dataLength);
					dataLength += 2;
				}
			}
			ti->dataInfo[fieldNo].offset = dataLength;
			dataLength += ti->dataInfo[fieldNo].type.length;
		}

		/* final check if entire fixed data block is within set boundary */
		if (record + dataLength > endOfSet) {
			msg(MSG_ERROR, "IpfixParser: Template record exceeds set boundary!");
			delete bt;
			return numberOfRecords;
		}

		/* Copy fixed data block */
		ti->data = (IpfixRecord::Data*)malloc(dataLength*sizeof(IpfixRecord::Data));
		ti->dataLength = dataLength;
		memcpy(ti->data, record, dataLength*sizeof(IpfixRecord::Data));
		 
		/* Advance record to end of fixed data block, i.e. start of next template record */
		record += dataLength;

		templateBuffer->bufferTemplate(bt); 
		if((sourceId->protocol == IPFIX_protocolIdentifier_UDP) && (templateLifetime > 0))
			bt->expires = time(0) + templateLifetime;
		else
			bt->expires = 0;
		
		IpfixTemplateRecord* ipfixRecord = templateRecordIM.getNewInstance();
		ipfixRecord->sourceID = sourceId;
		ipfixRecord->templateInfo = ti;
		push(ipfixRecord);
		numberOfRecords++;
	}
	return numberOfRecords;
}

/**
 * Processes an IPFIX data set.
 * Called by processMessage
 * returns number of processed records
 */
uint32_t IpfixParser::processDataSet(boost::shared_ptr<IpfixRecord::SourceID> sourceId, boost::shared_array<uint8_t> message, IpfixSetHeader* set, uint8_t* endOfMessage) {
	uint32_t numberOfRecords = 0;
	TemplateBuffer::BufferedTemplate* bt = templateBuffer->getBufferedTemplate(sourceId, ntohs(set->id));

	if (bt == 0) {
		/* this error may come in rapid succession; I hope I don't regret it */
		if(sourceId->exporterAddress.len == 4) {
			msg(MSG_INFO, "Template %d from %s unknown to collecting process", 
				ntohs(set->id), (sourceId->toString()).c_str());
		} else {
			msg(MSG_INFO, "Template %d from non-IPv4 unknown to collecting process", ntohs(set->id));
		}
		DPRINTF("Protocol: %u  Remote Port: %u", sourceId->protocol, sourceId->exporterPort);
		return 0;
	}
        
	uint8_t* endOfSet = (uint8_t*)set + ntohs(set->length);
	uint8_t* record = (uint8_t*)&set->data;

	/* check if set length lies within message boundaries */
	if (endOfSet > endOfMessage) {
		msg(MSG_ERROR, "IpfixParser: Data set exceeds message boundary!");
		return 0;
	}

#ifdef SUPPORT_NETFLOWV9
	if ((bt->templateInfo->setId == TemplateInfo::IpfixTemplate) || (bt->templateInfo->setId == TemplateInfo::IpfixOptionsTemplate) || (bt->templateInfo->setId == TemplateInfo::IpfixDataTemplate) || (bt->templateInfo->setId == TemplateInfo::NetflowTemplate)) {
#else
	if ((bt->templateInfo->setId == TemplateInfo::IpfixTemplate) || (bt->templateInfo->setId == TemplateInfo::IpfixOptionsTemplate) || (bt->templateInfo->setId == TemplateInfo::IpfixDataTemplate)) {
#endif

		boost::shared_ptr<TemplateInfo> ti = bt->templateInfo;
		bool hasStructuredData = false;
		bool hasStructuredDataInScopeField = false;

		// Determine if the template has any structured data elements - if it has
		// even in the case of a fixed length template a copy of the TemplateInfo
		// object has to be created for every entry.
		for (uint16_t i = 0; i < ti->fieldCount; i++) {
			if (ti->fieldInfo[i].type.isStructuredData()) {
				hasStructuredData = true;
				break;
			}
		}

		for (uint16_t i = 0; i < ti->scopeCount; i++) {
			if (ti->scopeInfo[i].type.isStructuredData()) {
				hasStructuredDataInScopeField = true;
				break;
			}
		}

		if (bt->recordLength < 65535) {
			if (record + bt->recordLength > endOfSet) {
				msg(MSG_ERROR, "IpfixParser: Got a Data Set that contained not a single full record");
			}
			else
			/* We stop processing when no full record is left */
			while (record + bt->recordLength <= endOfSet) {
				if (hasStructuredDataInScopeField || hasStructuredData) {
					// In case the template contains structured data the TemplateInfo
					// instance has to be copied as its FieldInfo entries will be
					// modified.
					ti = boost::shared_ptr<TemplateInfo>(bt->templateInfo.get());
				}

				bool error = false;

				if (hasStructuredData) {
					for (uint16_t i = 0; i < ti->fieldCount; i++) {
						if (!ti->fieldInfo[i].type.isStructuredData())
							continue;

						if (!processStructuredData(sourceId, &ti->fieldInfo[i], record + ti->fieldInfo[i].offset)) {
							error = true;
							break;
						}
					}
				}

				if (hasStructuredDataInScopeField) {
					for (uint16_t i = 0; i < ti->scopeCount; i++) {
						if (!ti->scopeInfo[i].type.isStructuredData())
							continue;

						if (!processStructuredData(sourceId, &ti->scopeInfo[i], record + ti->scopeInfo[i].offset)) {
							error = true;

							break;
						}
					}
				}

				if (error)
					break;

				IpfixDataRecord* ipfixRecord = dataRecordIM.getNewInstance();
				ipfixRecord->sourceID = sourceId;
				ipfixRecord->templateInfo = ti;
				ipfixRecord->dataLength = bt->recordLength;
				ipfixRecord->message = message;
				ipfixRecord->data = record;
				push(ipfixRecord);
				record = record + bt->recordLength;
				numberOfRecords++;
			}
		} else {
			/* Variable-length record */
			/* We assume that each field is at least 1 byte */
			/* scopeCount is zero for all Templates except Options Templates */
			if (record + ti->fieldCount + ti->scopeCount > endOfSet) {
				msg(MSG_ERROR, "IpfixParser: Got a Data Set that contained not a single full record");
			}
			else while (record < endOfSet) {
				int recordLength=0;
				int fieldLength;
				int i;
				bool incomplete = false;
				ti = boost::shared_ptr<TemplateInfo>(new TemplateInfo(*bt->templateInfo.get()));

				/* Go through scope fields first */
				for (i = 0; i < ti->scopeCount; i++) {
					if (!ti->scopeInfo[i].isVariableLength) {
						fieldLength = ti->scopeInfo[i].type.length;
					} else {
						/* check if 1 byte for the length lies within set boundary */
						if (record + recordLength + 1 > endOfSet) {
							incomplete = true;
							break;
						}
						fieldLength = *(uint8_t*)(record + recordLength);
						recordLength += 1;
						if (fieldLength == 255) {
							/* check if there are 2 bytes for the length */
							if (record + recordLength + 2 > endOfSet) {
								incomplete = true;
								break;
							}
							fieldLength = ntohs(*(uint16_t*)(record + recordLength));
							recordLength += 2;
						}
					}
					DPRINTF("Scope field: original length %u, offset %u", ti->fieldInfo[i].type.length, ti->fieldInfo[i].offset);
					ti->scopeInfo[i].offset = recordLength;
					ti->scopeInfo[i].type.length = fieldLength;

					// Handle structured data types
					if (ti->scopeInfo[i].type.isStructuredData())
						if (!processStructuredData(sourceId, &ti->scopeInfo[i], record + ti->scopeInfo[i].offset)) {
							incomplete = true;
							break;
						}

					recordLength += fieldLength;
				}

				/* final check if entire record is within set boundary */
				if (incomplete || (record + recordLength > endOfSet)) {
					DPRINTF("Incomplete variable length record");
					break;
				} 

				/* Now, go through non-scope fields */
				for (i = 0; i < ti->fieldCount; i++) {
					if (!ti->fieldInfo[i].isVariableLength) {
						fieldLength = ti->fieldInfo[i].type.length;
					} else {
						/* check if 1 byte for the length lies within set boundary */
						if (record + recordLength + 1 > endOfSet) {
							incomplete = true;
							break;
						}
						fieldLength = *(uint8_t*)(record + recordLength);
						recordLength += 1;
						if (fieldLength == 255) {
							/* check if there are 2 bytes for the length */
							if (record + recordLength + 2 > endOfSet) {
								incomplete = true;
								break;
							}
							fieldLength = ntohs(*(uint16_t*)(record + recordLength));
							recordLength += 2;
						}
					}
					DPRINTF("Non-scope field: original length %u, offset %u", ti->fieldInfo[i].type.length, ti->fieldInfo[i].offset);
					ti->fieldInfo[i].offset = recordLength;
					ti->fieldInfo[i].type.length = fieldLength;

					// Handle structured data types
					if (ti->fieldInfo[i].type.isStructuredData())
						if (!processStructuredData(sourceId, &ti->fieldInfo[i], record + ti->fieldInfo[i].offset)) {
							incomplete = true;
							break;
						}

					recordLength += fieldLength;
				}

				/* final check if entire record is within set boundary */
				if (incomplete || (record + recordLength > endOfSet)) {
					DPRINTF("Incomplete variable length record");
					break;
				} 

				IpfixDataRecord* ipfixRecord = dataRecordIM.getNewInstance();
				ipfixRecord->sourceID = sourceId;
				ipfixRecord->templateInfo = ti;
				ipfixRecord->dataLength = recordLength;
				ipfixRecord->message = message;
				ipfixRecord->data = record;
				push(ipfixRecord);
				record = record + recordLength;
				numberOfRecords++;
			}
		}
	} else {
	    msg(MSG_FATAL, "Data Set based on known but unhandled Template type %d", bt->templateInfo->setId);
	}
	return numberOfRecords;
}

/**
  * Processes a structured data field, invoking the corresponding parsing function.
  *
  * \param sourceId The template scope.
  * \param field The FieldInfo element to which this structured data element belongs.
  * \param data Pointer to the beginning of the structured data (e.g. the memory location at which the semantics field is located).
  * \returns True, if the structured data could be successfully parsed or if an unsupported type was found.
  *          If the structured data element was syntactically incorrect false is returned.
  */
bool IpfixParser::processStructuredData(boost::shared_ptr<IpfixRecord::SourceID> sourceId, TemplateInfo::FieldInfo *field, IpfixRecord::Data *data) {
	IpfixRecord::Data * const endOfData = data + field->type.length;

	if (data + sizeof(uint8_t) > endOfData) {
		DPRINTF("Structured data element too short to contain semantic field.");
		return false;
	}

	field->semantic = *data;

	switch (field->type.id) {
	case IPFIX_TYPEID_basicList:
		return processStructuredDataBasicList(sourceId, field, data, endOfData);
	case IPFIX_TYPEID_subTemplateList:
		return processStructuredDataSubTemplateList(sourceId, field, data, endOfData);
	case IPFIX_TYPEID_subTemplateMultiList:
		return processStructuredDataSetSubTemplateMultiList(sourceId, field, data, endOfData);
	}

	return true;
}


/**
  * Processes a subTemplateList structured data element storing meta-information about it
  * in its associated \a field.
  *
  * After this function finishes successfully the rows and rowInfo property of the given
  * \field is updated containing offsets (relative to the IpfixRecord's data property)
  * to the fields of the structured data.
  *
  * \param sourceId The template scope.
  * \param field The FieldInfo element to which this subTemplateList belongs.
  * \param data Pointer to the beginning of the subTemplateList (e.g. the memory location at which the semantics field is located).
  * \param endOfData Pointer to the end of the subTemplateList.
  * \returns True, if the subTemplateList could be successfully parsed, false otherwise.
  */
bool IpfixParser::processStructuredDataSubTemplateList(boost::shared_ptr<IpfixRecord::SourceID> sourceId, TemplateInfo::FieldInfo *field, IpfixRecord::Data *data, IpfixRecord::Data *endOfData) {
	// Minimum size of subTemplateList header is 3 bytes
	if (data + 3 > endOfData) {
		DPRINTF("Structured data element too short to contain subTemplateList header.");
		return false;
	}

	IpfixRecord::Data *const startOfList = data;

	// Skip semantics field
	data++;

	TemplateInfo::TemplateId templateId = ntohs(*((uint16_t *) data));
	data += sizeof(uint16_t);

	TemplateBuffer::BufferedTemplate *bt = templateBuffer->getBufferedTemplate(sourceId, templateId);

	if (bt == NULL) {
		DPRINTF("Could not find template with id %d for subTemplateList.", templateId);
		return false;
	}

	boost::shared_ptr<TemplateInfo> ti = bt->templateInfo;
	TemplateInfo::FieldInfo *const lastField = ti->fieldInfo + ti->fieldCount;

	// Count the number of rows in this subTemplateList
	IpfixRecord::Data *const startOfListContent = data;
	field->rowCount = 0;

	while (data < endOfData) {
		TemplateInfo::FieldInfo *currentField = ti->fieldInfo;

		while (currentField < lastField) {
			uint16_t elementLength = currentField->type.length;

			if (currentField->isVariableLength) {
				// Parse variable length
				if (data + sizeof(uint8_t) > endOfData) {
					DPRINTF("Structured data element too short to contain variable length information");
					break;
				}

				elementLength = *data;
				data++;

				if (elementLength == 0xff) {
					if (data + sizeof(uint16_t) > endOfData) {
						DPRINTF("Structured data element too short to contain extended variable length information");
						return false;
					}

					elementLength = ntohs(*((uint16_t *) data));
					data += sizeof(uint16_t);
				}

			}

			if (data + elementLength > endOfData) {
				DPRINTF("Structured data element too short to contain a field of length %d.", elementLength);
				return false;
			}

			data += elementLength;
			currentField++;
		}

		field->rowCount++;
	}


	field->rows = TemplateInfo::allocateStructuredDataRowArray(field->rowCount);

	if (field->rows == NULL) {
		msg(MSG_ERROR, "Failed to allocate memory for structured data rows.");
		return true;
	}

	TemplateInfo::StructuredDataRow *currentRow = field->rows;
	TemplateInfo::StructuredDataRow *const endOfRows = field->rows + field->rowCount;
	data = startOfListContent;

	while (currentRow < endOfRows) {
		currentRow->fieldCount = ti->fieldCount;
		currentRow->fields = TemplateInfo::allocateFieldInfoArray(currentRow->fieldCount);
		currentRow->templateId = ti->templateId;

		if (currentRow->fields == NULL)
			continue;

		TemplateInfo::FieldInfo *currentField = ti->fieldInfo;
		TemplateInfo::FieldInfo *currentTargetField = currentRow->fields;

		while (currentField < lastField) {
			// Copy template FieldInfo values into target
			memcpy(currentTargetField, currentField, sizeof(TemplateInfo::FieldInfo));

			uint16_t elementLength = currentField->type.length;

			if (currentField->isVariableLength) {
				elementLength = *data;
				data++;

				if (elementLength == 0xff) {
					elementLength = ntohs(*((uint16_t *) data));
					data += sizeof(uint16_t);
				}

				currentTargetField->type.length = elementLength;
			}

			currentTargetField->offset = field->offset + data - startOfList;

			if (currentTargetField->type.isStructuredData())
				processStructuredData(sourceId, currentTargetField, data);

			data += elementLength;

			currentField++;
			currentTargetField++;
		}

		currentRow++;
	}

	return true;
}

/**
  * Processes a subTemplateMultiList structured data element storing meta-information about it
  * in its associated \a field.
  *
  * \param sourceId The template scope.
  * \param field The FieldInfo element to which this subTemplateMultiList belongs.
  * \param data Pointer to the beginning of the subTemplateMultiList (e.g. the memory location at which the semantics field is located).
  * \param endOfData Pointer to the end of the subTemplateMultiList.
  * \returns True, if the subTemplateMultiList could be successfully parsed, false otherwise.
  */
bool IpfixParser::processStructuredDataSetSubTemplateMultiList(boost::shared_ptr<IpfixRecord::SourceID> sourceId, TemplateInfo::FieldInfo *field, IpfixRecord::Data *data, IpfixRecord::Data * const endOfData) {
	// Minimum size of subTemplateMultiList header is 3 bytes
	if (data + 3 > endOfData) {
		DPRINTF("Structured data element too short to contain subTemplateMultiList header.");
		return false;
	}

	IpfixRecord::Data *const startOfList = data;

	// Skip semantics field
	data++;

	// Count the number of rows in this subTemplateMultiList
	field->rowCount = 0;

	while (data < endOfData) {
		if (data + 4 > endOfData) {
			DPRINTF("Structured data element too short to contain template id and sub template length.");
			return false;
		}

		TemplateInfo::TemplateId templateId = ntohs(*((uint16_t *) data));
		data += sizeof(uint16_t);
		uint16_t subTemplateLength = ntohs(*((uint16_t *) data));
		data += sizeof(uint16_t);

		IpfixRecord::Data *const endOfTemplate = data - 4 + subTemplateLength; // Subtract 4 bytes for the templateId and the subTemplateLength field

		if (endOfTemplate > endOfData) {
			DPRINTF("Sub template entry points beyond end of packet.");
			return false;
		}

		TemplateBuffer::BufferedTemplate *bt = templateBuffer->getBufferedTemplate(sourceId, templateId);

		if (bt == NULL) {
			DPRINTF("Could not find template with id %d for subTemplateList.", templateId);
			return false;
		}

		boost::shared_ptr<TemplateInfo> ti = bt->templateInfo;
		TemplateInfo::FieldInfo *const lastField = ti->fieldInfo + ti->fieldCount;

		while (data < endOfTemplate) {
			TemplateInfo::FieldInfo *currentField = ti->fieldInfo;

			while (currentField < lastField) {
				uint16_t elementLength = currentField->type.length;

				if (currentField->isVariableLength) {
					// Parse variable length
					if (data + sizeof(uint8_t) > endOfData) {
						DPRINTF("Structured data element too short to contain variable length information");
						return false;
					}

					elementLength = *data;
					data++;

					if (elementLength == 0xff) {
						if (data + sizeof(uint16_t) > endOfData) {
							DPRINTF("Structured data element too short to contain extended variable length information");
							return false;
						}

						elementLength = ntohs(*((uint16_t *) data));
						data += sizeof(uint16_t);
					}

				}

				if (data + elementLength > endOfData) {
					DPRINTF("Structured data element too short to contain a field of length %d.", elementLength);
					return false;
				}

				data += elementLength;
				currentField++;
			}

			field->rowCount++;
		}
	}


	field->rows = TemplateInfo::allocateStructuredDataRowArray(field->rowCount);

	if (field->rows == NULL) {
		msg(MSG_ERROR, "Failed to allocate memory for structured data rows.");
		return true;
	}

	TemplateInfo::StructuredDataRow *currentRow = field->rows;
	data = startOfList + 1;

	while (data < endOfData) {
		TemplateInfo::TemplateId templateId = ntohs(*((uint16_t *) data));
		data += sizeof(uint16_t);
		uint16_t subTemplateLength = ntohs(*((uint16_t *) data));
		data += sizeof(uint16_t);

		TemplateBuffer::BufferedTemplate *bt = templateBuffer->getBufferedTemplate(sourceId, templateId);
		boost::shared_ptr<TemplateInfo> ti = bt->templateInfo;
		TemplateInfo::FieldInfo *const lastField = ti->fieldInfo + ti->fieldCount;
		IpfixRecord::Data *const endOfTemplate = data - 4 + subTemplateLength;

		while (data < endOfTemplate) {
			currentRow->templateId = ti->templateId;
			currentRow->fieldCount = ti->fieldCount;
			currentRow->fields = TemplateInfo::allocateFieldInfoArray(currentRow->fieldCount);
			if (currentRow->fields == NULL)
				continue;

			TemplateInfo::FieldInfo *currentField = ti->fieldInfo;
			TemplateInfo::FieldInfo *currentTargetField = currentRow->fields;

			while (currentField < lastField) {
				memcpy(currentTargetField, currentField, sizeof(TemplateInfo::FieldInfo));

				uint16_t elementLength = currentField->type.length;

				if (currentField->isVariableLength) {
					elementLength = *data;
					data++;

					if (elementLength == 0xff) {
						elementLength = ntohs(*((uint16_t *) data));
						data += sizeof(uint16_t);
					}

					currentTargetField->type.length = elementLength;
				}

				currentTargetField->offset = field->offset + data - startOfList;

				if (currentTargetField->type.isStructuredData())
					processStructuredData(sourceId, currentTargetField, data);

				data += elementLength;
				currentField++;
				currentTargetField++;
			}

			currentRow++;
		}
	}

	return true;
}


/**
  * Processes a basicList structured data element storing meta-information about it
  * in its associated \a field.
  *
  * \param sourceId The template scope.
  * \param field The FieldInfo element to which this basicList belongs.
  * \param data Pointer to the beginning of the basicList (e.g. the memory location at which the semantics field is located).
  * \param endOfData Pointer to the end of the basicList.
  * \returns True, if the basic list could be successfully parsed, false otherwise.
  */
bool IpfixParser::processStructuredDataBasicList(boost::shared_ptr<IpfixRecord::SourceID> sourceId, TemplateInfo::FieldInfo *field, IpfixRecord::Data *data, IpfixRecord::Data * const endOfData) {
	// Minimum size of basicList header is 5 bytes
	if (data + 5 > endOfData) {
		DPRINTF("Structured data element too short to contain basicList header.");
		return false;
	}

	IpfixRecord::Data *const startOfList = data;

	// Skip semantics field
	data++;

	uint32_t enterpriseNumber = 0;
	uint16_t informationElementId = ntohs(*((uint16_t *) data));
	data += sizeof(uint16_t);

	uint16_t elementLength = ntohs(*((uint16_t *) data));
	data += sizeof(uint16_t);

	bool isVariableLength = elementLength == 0xffff;

	if (informationElementId & IPFIX_ENTERPRISE_TYPE) {
		if (data + sizeof(uint32_t) > endOfData) {
			DPRINTF("Structured data element too short to contain enterprise number.");
			return false;
		}

		enterpriseNumber = ntohl(*((uint32_t *) data));
		data += sizeof(uint32_t);

		informationElementId &= ~IPFIX_ENTERPRISE_TYPE;
	}

	IpfixRecord::Data *const startOfListContent = data;

	field->rowCount = 0;

	// Count number of rows and perform sanity checking
	while (data < endOfData) {
		if (isVariableLength) {
			if (data + sizeof(uint8_t) > endOfData) {
				DPRINTF("Structured data element too short to contain variable length information");
				return false;
			}

			elementLength = *data;
			data++;

			if (elementLength == 0xff) {
				if (data + sizeof(uint16_t) > endOfData) {
					DPRINTF("Structured data element too short to contain extended variable length information");
					return false;
				}

				elementLength = ntohs(*((uint16_t *) data));
				data += sizeof(uint16_t);
			}
		}

		if (data + elementLength > endOfData) {
			DPRINTF("Structured data element too short to contain a field of length %d.", elementLength);
			return false;
		}

		data += elementLength;
		field->rowCount++;
	}

	field->rows = TemplateInfo::allocateStructuredDataRowArray(field->rowCount);

	if (field->rows == NULL) {
		field->rowCount = 0;

		msg(MSG_ERROR, "Failed to allocate memory for structured data rows.");
		return false;
	}

	TemplateInfo::StructuredDataRow *currentRow = field->rows;
	TemplateInfo::StructuredDataRow *const endOfRows = field->rows + field->rowCount;
	data = startOfListContent;

	while (currentRow < endOfRows) {
		currentRow->fieldCount = 1;
		currentRow->fields = TemplateInfo::allocateFieldInfoArray(1);

		if (currentRow->fields == NULL) {
			return false;
		}

		if (isVariableLength) {
			elementLength = *data;
			data++;

			if (elementLength == 0xff) {
				elementLength = ntohs(*((uint16_t *) data));
				data += sizeof(uint16_t);
			}
		}

		TemplateInfo::FieldInfo *fieldInfo = currentRow->fields;

		fieldInfo->isVariableLength = isVariableLength;
		fieldInfo->offset = field->offset + (data - startOfList);
		fieldInfo->type.enterprise = enterpriseNumber;
		fieldInfo->type.id = informationElementId;
		fieldInfo->type.length = elementLength;

		if (fieldInfo->type.isStructuredData())
			if (!processStructuredData(sourceId, fieldInfo, data))
				return false;

		data += elementLength;

		currentRow++;
	}

	return true;

}

        
/**
 * Process a NetflowV9 Packet
 * @return 0 on success
 */
int IpfixParser::processNetflowV9Packet(boost::shared_array<uint8_t> message, uint16_t length, boost::shared_ptr<IpfixRecord::SourceID> sourceId) 
{
	if (length < sizeof(NetflowV9Header)) {
		msg(MSG_ERROR, "IpfixParser: Invalide NetFlowV9 message - message too short to contain header!");
		return -1;
	}
	
	NetflowV9Header* header = (NetflowV9Header*)message.get();
	sourceId->observationDomainId = ntohl(header->observationDomainId);
	sourceId->sysUpTime = ntohl(header->upTime);
	sourceId->exportTime = ntohl(header->exportTime);

	/* pointer to first set */
	IpfixSetHeader* set = (IpfixSetHeader*)&header->data;

	/* pointer beyond message */
	uint8_t* endOfMessage = (uint8_t*)((uint8_t*)message.get() + length); 

	uint16_t tmpid;
	uint16_t expectedNumberOfRecords = ntohs(header->setCount);
	uint16_t numberOfDataRecords = 0;
	uint16_t numberOfTemplateRecords = 0;
	uint32_t sequenceNumber = ntohl(header->sequenceNo);

	while (((numberOfDataRecords + numberOfTemplateRecords) <= expectedNumberOfRecords) && (((uint8_t*)(set) + 4) <= endOfMessage)) {
		/* check set length */
		if (ntohs(set->length) < 3) {
			msg(MSG_ERROR, "IpfixParser: Invalid set length %u, must be >= 4", ntohs(set->length));
			return -1;
		}

		tmpid=ntohs(set->id);

		switch(tmpid) {
			case NetflowV9_SetId_Template:
				numberOfTemplateRecords += processTemplateSet(sourceId, TemplateInfo::NetflowTemplate, message, set, endOfMessage);
				break;
			case NetflowV9_SetId_OptionsTemplate:
				numberOfTemplateRecords += processOptionsTemplateSet(sourceId, TemplateInfo::IpfixOptionsTemplate, message, set, endOfMessage);
				break;
			default:
				if(tmpid >= IPFIX_SetId_Data_Start) {
					numberOfDataRecords += processDataSet(sourceId, message, set, endOfMessage);
				} else {
					msg(MSG_ERROR, "processNetflowV9Packet: Unsupported Set ID - expected 0/1/256+, got %d", tmpid);
				}
		}
		set = (IpfixSetHeader*)((uint8_t*)set + ntohs(set->length));
	}

	/* check if there are trailing bytes */
	if ((uint8_t*)(set) != endOfMessage) {
		msg(MSG_ERROR, "IpfixParser: NetFlowV9 message contains %u trailing bytes!", endOfMessage - (uint8_t*)(set));
	}
	/* check if we got all records */
	if ((numberOfDataRecords + numberOfTemplateRecords) != expectedNumberOfRecords) {
		msg(MSG_INFO, "IpfixParser: NetFlowV9 message header indicates %u records, but there were only %u records! Maybe the Template is unknown.", expectedNumberOfRecords, numberOfDataRecords+numberOfTemplateRecords);
	}
	
	// detect and count data record losses
	//FIXME: detect lost records in the case of PR-SCTP (considering SCTP stream id)
	if(sourceId->protocol == 17) {
		std::map<IpfixRecord::SourceID, SNInfo>::iterator iter = snInfoMap.find(*sourceId.get());
		if(iter != snInfoMap.end()) {
			int32_t difference = sequenceNumber - iter->second.expectedSN; 
			if(difference > 0) {
				msg(MSG_INFO, "IpfixParser: Loss of %d NetflowV9 messages from %s detected (SN=%u, expected=%u).",
					difference, (sourceId->toString()).c_str(), sequenceNumber, iter->second.expectedSN);
				iter->second.lostMessages += difference;
			} else if (difference < 0) {
				msg(MSG_INFO, "IpfixParser: Out-of-order or repeated NetflowV9 message detected from %s (SN=%u, expected=%u).",
					(sourceId->toString()).c_str(), sequenceNumber, iter->second.expectedSN);
				iter->second.outOfOrderMessages++;
			}
			iter->second.expectedSN = sequenceNumber + 1;
			iter->second.receivedMessages++;
			iter->second.receivedDataRecords += numberOfDataRecords;
			iter->second.receivedTemplateRecords += numberOfTemplateRecords;
		} else {
			SNInfo newSnInfo;
			newSnInfo.expectedSN = sequenceNumber + 1;
			newSnInfo.receivedMessages = 1;
			newSnInfo.receivedDataRecords = numberOfDataRecords;
			newSnInfo.receivedTemplateRecords = numberOfTemplateRecords;
			snInfoMap[*sourceId.get()] = newSnInfo;
		}
	}

	msg(MSG_VDEBUG, "NetflowV9 message from %s contained %u Data Records and %u Template Records. Sequence number was %lu.", 
		(sourceId->toString()).c_str(), numberOfDataRecords, numberOfTemplateRecords, (unsigned long) sequenceNumber);

	// Update statistics
	statTotalMessages++;
	statTotalDataRecords += numberOfDataRecords;
	statTotalTemplateRecords += numberOfTemplateRecords;

	return 0;
}

/**
 * Process an IPFIX Packet
 * @return 0 on success
 */
int IpfixParser::processIpfixPacket(boost::shared_array<uint8_t> message, uint16_t length, boost::shared_ptr<IpfixRecord::SourceID> sourceId)
{
	if (length < sizeof(IpfixHeader)) {
		msg(MSG_ERROR, "IpfixParser: Invalide IPFIX message - message too short to contain header!");
		return -1;
	}
	IpfixHeader* header = (IpfixHeader*)message.get();
	sourceId->observationDomainId = ntohl(header->observationDomainId);
	sourceId->exportTime = ntohl(header->exportTime);

	if (ntohs(header->length) != length) {
		msg(MSG_ERROR, "IpfixParser: Bad message length - packet length is  %#06x, header length field is %#06x\n", length, ntohs(header->length));
		return -1;
	}

	/* pointer to first set */
	IpfixSetHeader* set = (IpfixSetHeader*)&header->data;

	/* pointer beyond message */
	uint8_t* endOfMessage = (uint8_t*)((uint8_t*)message.get() + length); 

	uint16_t tmpid;
	uint16_t numberOfDataRecords = 0;
	uint16_t numberOfTemplateRecords = 0;
	uint32_t sequenceNumber = ntohl(header->sequenceNo);

	/* while there is space for a set header... */
	while((uint8_t*)(set) + 4 <= endOfMessage) {
		/* check set length */
		if (ntohs(set->length) < 3) {
			msg(MSG_ERROR, "IpfixParser: Invalid set length %u, must be >= 4", ntohs(set->length));
			return -1;
		}

		tmpid=ntohs(set->id);

		switch(tmpid) {
			case IPFIX_SetId_DataTemplate:
				numberOfTemplateRecords += processDataTemplateSet(sourceId, message, set, endOfMessage);
				break;
			case IPFIX_SetId_Template:
				numberOfTemplateRecords += processTemplateSet(sourceId, TemplateInfo::IpfixTemplate, message, set, endOfMessage);
				break;
			case IPFIX_SetId_OptionsTemplate:
				numberOfTemplateRecords += processOptionsTemplateSet(sourceId, TemplateInfo::IpfixOptionsTemplate, message, set, endOfMessage);
				break;
			default:
				if(tmpid >= IPFIX_SetId_Data_Start) {
					numberOfDataRecords += processDataSet(sourceId, message, set, endOfMessage);
				} else {
					msg(MSG_ERROR, "processIpfixPacket: Unsupported Set ID - expected 2/3/4/256+, got %d", tmpid);
				}
		}
		set = (IpfixSetHeader*)((uint8_t*)set + ntohs(set->length));
	}
	
	// detect and count data record losses
	//FIXME: detect lost records in the case of PR-SCTP (considering SCTP stream id)
	if(sourceId->protocol == 17) {
		std::map<IpfixRecord::SourceID, SNInfo>::iterator iter = snInfoMap.find(*sourceId.get());
		if(iter != snInfoMap.end()) {
			int32_t difference = sequenceNumber - iter->second.expectedSN; 
			if(difference > 0) {
				msg(MSG_INFO, "IpfixParser: Loss of %d IPFIX Data Records from %s detected (SN=%u, expected=%u).",
					difference, (sourceId->toString()).c_str(), sequenceNumber, iter->second.expectedSN);
				iter->second.lostDataRecords += difference;
			} else if (difference < 0) {
				msg(MSG_INFO, "IpfixParser: Out-of-order or repeated IPFIX message detected from %s (SN=%u, expected=%u).",
					(sourceId->toString()).c_str(), sequenceNumber, iter->second.expectedSN);
				iter->second.outOfOrderMessages ++;
			}
			iter->second.expectedSN = sequenceNumber + numberOfDataRecords;
			iter->second.receivedMessages++;
			iter->second.receivedDataRecords += numberOfDataRecords;
			iter->second.receivedTemplateRecords += numberOfTemplateRecords;
		} else {
			SNInfo newSnInfo;
			newSnInfo.expectedSN = sequenceNumber + numberOfDataRecords;
			newSnInfo.receivedMessages = 1;
			newSnInfo.receivedDataRecords = numberOfDataRecords;
			newSnInfo.receivedTemplateRecords = numberOfTemplateRecords;
			snInfoMap[*sourceId.get()] = newSnInfo;
		}
	}

	msg(MSG_VDEBUG, "IPFIX message from %s contained %u Data Records and %u Template Records. Sequence number was %lu.", 
		(sourceId->toString()).c_str(), numberOfDataRecords, numberOfTemplateRecords, (unsigned long) sequenceNumber);

	// Update statistics
	statTotalMessages++;
	statTotalDataRecords += numberOfDataRecords;
	statTotalTemplateRecords += numberOfTemplateRecords;

	return 0;
}

/**
 * Process new Message
 * @return 0 on success
 */
int IpfixParser::processPacket(boost::shared_array<uint8_t> message, uint16_t length, boost::shared_ptr<IpfixRecord::SourceID> sourceId)
{
	pthread_mutex_lock(&mutex);
	if (length == 0) {
		templateBuffer->destroyBufferedTemplate(sourceId, 0, true);
		pthread_mutex_unlock(&mutex);
		return 0;
	}
	IpfixHeader* header = (IpfixHeader*)message.get();
	if (ntohs(header->version) == 0x000a) {
		int r = processIpfixPacket(message, length, sourceId);
		pthread_mutex_unlock(&mutex);
		return r;
	}
#ifdef SUPPORT_NETFLOWV9
	if (ntohs(header->version) == 0x0009) {
		int r = processNetflowV9Packet(message, length, sourceId);
		pthread_mutex_unlock(&mutex);
		return r;
	}
	DPRINTF("Bad message version - expected 0x009 or 0x000a, got %#06x\n", ntohs(header->version));
	pthread_mutex_unlock(&mutex);
	return -1;
#else
	DPRINTF("Bad message version - expected 0x000a, got %#06x\n", ntohs(header->version));
	pthread_mutex_unlock(&mutex);
	return -1;
#endif
}
	
/**
 * Creates a new  @c IpfixParser.
 * @return handle to created instance
 */
IpfixParser::IpfixParser(IpfixRecordSender* sender) 
	: templateLifetime(DEFAULT_TEMPLATE_EXPIRE_SECS),
	  statTotalDataRecords(0),
	  statTotalTemplateRecords(0),
  	  statTotalMessages(0),
  	  ipfixRecordSender(sender)
{

	if (pthread_mutex_init(&mutex, NULL) != 0) {
		msg(MSG_FATAL, "Could not init mutex");
		THROWEXCEPTION("IpfixParser creation failed");
	}

	templateBuffer = new TemplateBuffer(this);

	SensorManager::getInstance().addSensor(this, "IpfixParser", 0);
}


/**
 * Frees memory used by an IpfixParser.
 */
IpfixParser::~IpfixParser() {

	delete(templateBuffer);

	pthread_mutex_destroy(&mutex);
	SensorManager::getInstance().removeSensor(this);

}


/**
 * statistics function called by StatisticsManager
 */
std::string IpfixParser::getStatisticsXML(double interval)
{
	ostringstream oss;
	
	oss << "<totalDataRecords>" << statTotalDataRecords << "</totalDataRecords>";
	oss << "<totalTemplateRecords>" << statTotalTemplateRecords << "</totalTemplateRecords>";
	oss << "<totalMessages>" << statTotalMessages << "</totalMessages>";

	for(std::map<IpfixRecord::SourceID, SNInfo>::iterator iter = snInfoMap.begin(); iter != snInfoMap.end(); iter++) {
		oss << "<exporter><sourceId>" << iter->first.toString() << "</sourceId>";
		oss << "<receivedMessages>" << iter->second.receivedMessages << "</receivedMessages>";   
		oss << "<receivedDataRecords>" << iter->second.receivedDataRecords << "</receivedDataRecords>";   
		oss << "<receivedTemplateRecords>" << iter->second.receivedTemplateRecords << "</receivedTemplateRecords>";   
		if(iter->second.lostMessages > 0)
			oss << "<lostMessages>" << iter->second.lostMessages << "</lostMessages>";   
		if(iter->second.outOfOrderMessages > 0)
			oss << "<outOfOrderMessages>" << iter->second.outOfOrderMessages << "</outOfOrderMessages>";   
		if(iter->second.lostDataRecords > 0)
			oss << "<lostDataRecords>" << iter->second.lostDataRecords << "</lostDataRecords>";   
		oss << "</exporter>";
	}
        return oss.str();
}

/**
 * function push overwritten from FlowSource
 */
void IpfixParser::push(IpfixRecord* ipfixRecord)
{
	ipfixRecordSender->send(ipfixRecord);
}


void IpfixParser::performStart()
{
}

void IpfixParser::performShutdown()
{
	// we do not need to withdraw template since every module should delete stored templates during reconfiguration and shutdown
	// withdrawBufferedTemplates();
}

void IpfixParser::preReconfiguration()
{
	// we do not need to withdraw templates because every module should delete stored templates during reconfiguration anyway
	// withdrawBufferedTemplates();
}

void IpfixParser::onReconfiguration1()
{
}

/**
 * event forwarded from IpfixCollector, called after reconfiguration
 */
void IpfixParser::postReconfiguration()
{
	// we must resend all buffered templates
	resendBufferedTemplates();
}

/**
 * resends all buffered templates to following modules
 */
void IpfixParser::resendBufferedTemplates()
{
	TemplateBuffer::BufferedTemplate* bt = templateBuffer->getFirstBufferedTemplate();
		
	while (bt) {	
		IpfixTemplateRecord* ipfixRecord = templateRecordIM.getNewInstance();
		ipfixRecord->sourceID = bt->sourceID;
		ipfixRecord->templateInfo = bt->templateInfo;
		push(ipfixRecord);
		
		bt = bt->next;
	}
}

/**
 * withdraws all buffered templates
 */
void IpfixParser::withdrawBufferedTemplates()
{
	TemplateBuffer::BufferedTemplate* bt = templateBuffer->getFirstBufferedTemplate();
		
	while (bt) {	
		IpfixTemplateDestructionRecord* ipfixRecord = templateDestructionRecordIM.getNewInstance();
		ipfixRecord->sourceID = bt->sourceID;
		ipfixRecord->templateInfo = bt->templateInfo;
		push(ipfixRecord);
		
		bt = bt->next;
	}
}

