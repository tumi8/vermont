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

bool IpfixParser::isWithinTimeBoundary(uint32_t exportTime)
{
#ifdef EXPORT_TIME_SANITY_CHECK
	uint32_t currentTime;
	currentTime = static_cast<uint32_t>(time(NULL));  
	/* If either the flow is more than 1h in the future, or older than 1 day on arrival 
	 * it is probably a bad exporter so reject it
	*/
	if(((currentTime - exportTime + 3600 ) < 0) ||
		((currentTime - exportTime) > 86400) )  {
		return false;
	}
#endif
	return true;
}

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
		ti->fieldInfo = (TemplateInfo::FieldInfo*)malloc(ti->fieldCount * sizeof(TemplateInfo::FieldInfo));
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
			// IPFIX only: If highest bit of field id is set (0x8000), we must look for an enterprise number.
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
		if (setId == TemplateInfo::IpfixOptionsTemplate) {
			ti->scopeCount = ntohs(oth->scopeCount);
			ti->fieldCount = ntohs(oth->fieldCount)-ntohs(oth->scopeCount);
		} else if (setId == TemplateInfo::NetflowOptionsTemplate) {
			/* NB: for NetflowV9, scopeCount<->fieldCount are swapped, and
			 * contain the length of scopes/fields in bytes. Each one is
			 * 4 bytes long. */
			ti->scopeCount = ntohs(oth->fieldCount)/4;
			ti->fieldCount = ntohs(oth->scopeCount)/4;
		}
		ti->scopeInfo = (TemplateInfo::FieldInfo*)malloc((ti->scopeCount) * sizeof(TemplateInfo::FieldInfo));
		ti->fieldInfo = (TemplateInfo::FieldInfo*)malloc((ti->fieldCount) * sizeof(TemplateInfo::FieldInfo));
		int isLengthVarying = 0;
		uint16_t scopeNo = 0;
		for (scopeNo = 0; scopeNo < ti->scopeCount; scopeNo++) {
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
			// IPFIX only: If highest bit of field id is set (0x8000), we must look for an enterprise number.
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
		}

		uint16_t fieldNo = 0;
		for (fieldNo = 0; fieldNo < ti->fieldCount; fieldNo++) {
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
			// IPFIX only: If highest bit of field id is set (0x8000), we must look for an enterprise number.
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
		ti->fieldInfo = (TemplateInfo::FieldInfo*)malloc(ti->fieldCount * sizeof(TemplateInfo::FieldInfo));
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
	if ((bt->templateInfo->setId == TemplateInfo::IpfixTemplate) || (bt->templateInfo->setId == TemplateInfo::IpfixOptionsTemplate) || (bt->templateInfo->setId == TemplateInfo::IpfixDataTemplate) || (bt->templateInfo->setId == TemplateInfo::NetflowTemplate) || (bt->templateInfo->setId == TemplateInfo::NetflowOptionsTemplate)) {
#else
	if ((bt->templateInfo->setId == TemplateInfo::IpfixTemplate) || (bt->templateInfo->setId == TemplateInfo::IpfixOptionsTemplate) || (bt->templateInfo->setId == TemplateInfo::IpfixDataTemplate)) {
#endif

		boost::shared_ptr<TemplateInfo> ti = bt->templateInfo;
        
		if (bt->recordLength < 65535) {
			if (record + bt->recordLength > endOfSet) {
				msg(MSG_ERROR, "IpfixParser: Got a Data Set that contained not a single full record");
			}
			else
			/* We stop processing when no full record is left */
			while (record + bt->recordLength <= endOfSet) {
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
 * Process a NetflowV9 Packet
 * @return 0 on success
 */
int IpfixParser::processNetflowV9Packet(boost::shared_array<uint8_t> message, uint16_t length, boost::shared_ptr<IpfixRecord::SourceID> sourceId) 
{
	if (length < sizeof(NetflowV9Header)) {
		msg(MSG_ERROR, "IpfixParser: Invalid NetFlowV9 message - message too short to contain header!");
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
				numberOfTemplateRecords += processOptionsTemplateSet(sourceId, TemplateInfo::NetflowOptionsTemplate, message, set, endOfMessage);
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
		if (!isWithinTimeBoundary(ntohl(header->exportTime))) {
			uint32_t currentTime = static_cast<uint32_t>(time(NULL));  
			msg(MSG_ERROR, "Received old message. Current time is %u. Message time is %u", currentTime, ntohl(header->exportTime));
			pthread_mutex_unlock(&mutex);
			return -1;
		}
		int r = processIpfixPacket(message, length, sourceId);
		pthread_mutex_unlock(&mutex);
		return r;
	}
#ifdef SUPPORT_NETFLOWV9
	if (ntohs(header->version) == 0x0009) {
		NetflowV9Header* nfHeader = (NetflowV9Header*)message.get();
		if (!isWithinTimeBoundary(ntohl(nfHeader->exportTime))) {
			uint32_t currentTime = static_cast<uint32_t>(time(NULL));  
			msg(MSG_ERROR, "Received old message. Current time is %u. Message time is %u", currentTime, ntohl(nfHeader->exportTime));
			pthread_mutex_unlock(&mutex);
			return -1;
		}
		int r = processNetflowV9Packet(message, length, sourceId);
		pthread_mutex_unlock(&mutex);
		return r;
	}
	msg(MSG_ERROR, "Bad message version - expected 0x009 or 0x000a, got %#06x\n", ntohs(header->version));
	pthread_mutex_unlock(&mutex);
	return -1;
#else
	msg(MSG_ERROR, "Bad message version - expected 0x000a, got %#06x\n", ntohs(header->version));
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

