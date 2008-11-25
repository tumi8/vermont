/*
 * IPFIX Concentrator Module Library
 * Copyright (C) 2004 Christoph Sommer <http://www.deltadevelopment.de/users/christoph/ipfix/>
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

// FIXME: Basic support for NetflowV9 packets, templates and flow records is provided. Will break when fed field types with type ID >= 0x8000.

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sstream>

/* for ntohll et al */
#include "ipfixlolib/ipfixlolib.h"

#include "IpfixReceiver.hpp"
#include "IpfixParser.hpp"
#include "TemplateBuffer.hpp"
#include "ipfix.hpp"
#include "IpfixPrinter.hpp"

#include "common/msg.h"
#include "common/Misc.h"

using namespace std;

#define MAX_MSG_LEN 65536

//static variables
InstanceManager<IpfixTemplateRecord> IpfixParser::templateRecordIM("IpfixTemplateRecord", 0);
InstanceManager<IpfixOptionsTemplateRecord> IpfixParser::optionsTemplateRecordIM("IpfixOptionsTemplateRecord", 0);
InstanceManager<IpfixDataTemplateRecord> IpfixParser::dataTemplateRecordIM("IpfixDataTemplateRecord", 0);		
InstanceManager<IpfixDataRecord> IpfixParser::dataRecordIM("IpfixDataRecord", 0);
InstanceManager<IpfixOptionsRecord> IpfixParser::optionsRecordIM("IpfixOptionsRecord", 0);
InstanceManager<IpfixDataDataRecord> IpfixParser::dataDataRecordIM("IpfixDataDataRecord", 0);
InstanceManager<IpfixTemplateDestructionRecord> IpfixParser::templateDestructionRecordIM("IpfixTemplateDestructionRecord", 0);
InstanceManager<IpfixOptionsTemplateDestructionRecord> IpfixParser::optionsTemplateDestructionRecordIM("IpfixOptionsTemplateDestructionRecord", 0);
InstanceManager<IpfixDataTemplateDestructionRecord> IpfixParser::dataTemplateDestructionRecordIM("IpfixDataTemplateDestructionRecord", 0);


/**
 * Processes an IPFIX template set.
 * Called by processMessage
 */
void IpfixParser::processTemplateSet(boost::shared_ptr<IpfixRecord::SourceID> sourceId, boost::shared_array<uint8_t> message, IpfixSetHeader* set, uint8_t* endOfMessage) {
	uint8_t* endOfSet = (uint8_t*)set + ntohs(set->length);
	uint8_t* record = (uint8_t*)&set->data;

	/* check if set length lies within message boundaries */
	if (endOfSet > endOfMessage) {
		msg(MSG_ERROR, "IpfixParser: Template set exceeds message boundary!");
		return;
	}

	/* Template records are >= 4 byte, so we stop processing when fewer bytes are left */
	while (record + 4 <= endOfSet) {
		IpfixTemplateHeader* th = (IpfixTemplateHeader*)record;
		record = (uint8_t*)&th->data;
		if (th->fieldCount == 0) {
			/* This is a Template withdrawal message */
			templateBuffer->destroyBufferedTemplate(sourceId, ntohs(th->templateId));
			continue;
		}
		TemplateBuffer::BufferedTemplate* bt = new TemplateBuffer::BufferedTemplate;
		boost::shared_ptr<IpfixRecord::TemplateInfo> ti(new IpfixRecord::TemplateInfo);
		bt->sourceID = sourceId;
		bt->templateID = ntohs(th->templateId);
		bt->recordLength = 0;
		bt->setID = ntohs(set->id);
		bt->templateInfo = ti;
		ti->userData = 0;
		ti->templateId = ntohs(th->templateId);
		ti->fieldCount = ntohs(th->fieldCount);
		ti->fieldInfo = (IpfixRecord::FieldInfo*)malloc(ti->fieldCount * sizeof(IpfixRecord::FieldInfo));
		int isLengthVarying = 0;
		uint16_t fieldNo;
		for (fieldNo = 0; fieldNo < ti->fieldCount; fieldNo++) {
			/* check if there are at least 4 bytes for this field */
			if (record+4 > endOfSet) {
				msg(MSG_ERROR, "IpfixParser: Template record (id=%u) exceeds set boundary!", bt->templateID);
				delete bt;
				return;
			}
			ti->fieldInfo[fieldNo].type.id = ntohs(*(uint16_t*)((uint8_t*)record+0));
			ti->fieldInfo[fieldNo].type.length = ntohs(*(uint16_t*)((uint8_t*)record+2));
			ti->fieldInfo[fieldNo].type.isVariableLength = (ti->fieldInfo[fieldNo].type.length == 65535);
			ti->fieldInfo[fieldNo].offset = bt->recordLength; 
			bt->recordLength+=ti->fieldInfo[fieldNo].type.length;
			if (ti->fieldInfo[fieldNo].type.length == 65535) {
				isLengthVarying=1;
			}
			if (ti->fieldInfo[fieldNo].type.id & IPFIX_ENTERPRISE_TYPE) {
				/* check if there are 8 bytes for this field */
				if (record+8 > endOfSet) {
					msg(MSG_ERROR, "IpfixParser: Template record (id=%u) exceeds set boundary!", bt->templateID);
					delete bt;
					return;
				}
				ti->fieldInfo[fieldNo].type.eid = ntohl(*(uint32_t*)((uint8_t*)record+4));
				record = (uint8_t*)((uint8_t*)record+8);
			} else {
				ti->fieldInfo[fieldNo].type.eid = 0;
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
		if((sourceId->protocol == IPFIX_protocolIdentifier_UDP) && (templateLivetime > 0))
			bt->expires = time(0) + templateLivetime;
		else
			bt->expires = 0;

		IpfixTemplateRecord* ipfixRecord = templateRecordIM.getNewInstance();
		ipfixRecord->sourceID = sourceId;
		ipfixRecord->templateInfo = ti;
		push(ipfixRecord);
	}
}

/**
 * Processes an IPFIX Options Template Set.
 * Called by processMessage
 */
void IpfixParser::processOptionsTemplateSet(boost::shared_ptr<IpfixRecord::SourceID> sourceId, boost::shared_array<uint8_t> message, IpfixSetHeader* set, uint8_t* endOfMessage) {
	uint8_t* endOfSet = (uint8_t*)set + ntohs(set->length);
	uint8_t* record = (uint8_t*)&set->data;

	/* check if set length lies within message boundaries */
	if (endOfSet > endOfMessage) {
		msg(MSG_ERROR, "IpfixParser: Option template set exceeds message boundary!");
		return;
	}

	/* Options template records are >= 4 byte, so we stop processing when fewer are left */
	while (record + 4 <= endOfSet) {
		/* First cast to normal template header used for withdrawal message */
		IpfixTemplateHeader* th = (IpfixTemplateHeader*)record;
		record = (uint8_t*)&th->data;
		if (th->fieldCount == 0) {
			/* This is a Template withdrawal message */
			templateBuffer->destroyBufferedTemplate(sourceId, ntohs(th->templateId));
			continue;
		}

		IpfixOptionsTemplateHeader* oth = (IpfixOptionsTemplateHeader*)th;
		record = (uint8_t*)&oth->data;

		/* Non-withdrawal options template records are >= 6 byte */
		if (record > endOfSet) {
			DPRINTF("Strange long padding in option template");
			return; 
		}

		TemplateBuffer::BufferedTemplate* bt = new TemplateBuffer::BufferedTemplate;
		boost::shared_ptr<IpfixRecord::OptionsTemplateInfo> ti(new IpfixRecord::OptionsTemplateInfo);
		bt->sourceID = sourceId;
		bt->templateID = ntohs(oth->templateId);
		bt->recordLength = 0;
		bt->setID = ntohs(set->id);
		bt->optionsTemplateInfo = ti;
		ti->userData = 0;
		ti->templateId = ntohs(oth->templateId);
		ti->scopeCount = ntohs(oth->scopeCount);
		ti->scopeInfo = (IpfixRecord::FieldInfo*)malloc(ti->scopeCount * sizeof(IpfixRecord::FieldInfo));
		ti->fieldCount = ntohs(oth->fieldCount)-ntohs(oth->scopeCount);
		ti->fieldInfo = (IpfixRecord::FieldInfo*)malloc(ti->fieldCount * sizeof(IpfixRecord::FieldInfo));
		int isLengthVarying = 0;
		uint16_t scopeNo;
		for (scopeNo = 0; scopeNo < ti->scopeCount; scopeNo++) {
			/* check if there are at least 4 bytes for this field */
			if (record+4 > endOfSet) {
				msg(MSG_ERROR, "IpfixParser: Option template record exceeds set boundary!");
				delete bt;
				return;
			}
			ti->scopeInfo[scopeNo].type.id = ntohs(*(uint16_t*)((uint8_t*)record+0));
			ti->scopeInfo[scopeNo].type.length = ntohs(*(uint16_t*)((uint8_t*)record+2));
			ti->scopeInfo[scopeNo].type.isVariableLength = (ti->scopeInfo[scopeNo].type.length == 65535);
			ti->scopeInfo[scopeNo].offset = bt->recordLength; 
			bt->recordLength+=ti->scopeInfo[scopeNo].type.length;
			if (ti->scopeInfo[scopeNo].type.length == 65535) {
				isLengthVarying=1;
			}
			if (ti->scopeInfo[scopeNo].type.id & IPFIX_ENTERPRISE_TYPE) {
				/* check if there are 8 bytes for this field */
				if (record+8 > endOfSet) {
					msg(MSG_ERROR, "IpfixParser: Option template record exceeds set boundary!");
					delete bt;
					return;
				}
				ti->scopeInfo[scopeNo].type.eid = ntohl(*(uint32_t*)((uint8_t*)record+4));
				record = (uint8_t*)((uint8_t*)record+8);
			} else {
				ti->fieldInfo[scopeNo].type.eid = 0;
				record = (uint8_t*)((uint8_t*)record+4);
			}
		}
		uint16_t fieldNo;
		for (fieldNo = 0; fieldNo < ti->fieldCount; fieldNo++) {
			/* check if there are at least 4 bytes for this field */
			if (record+4 > endOfSet) {
				msg(MSG_ERROR, "IpfixParser: Template record exceeds set boundary!");
				delete bt;
				return;
			}
			ti->fieldInfo[fieldNo].type.id = ntohs(*(uint16_t*)((uint8_t*)record+0));
			ti->fieldInfo[fieldNo].type.length = ntohs(*(uint16_t*)((uint8_t*)record+2));
			ti->fieldInfo[fieldNo].type.isVariableLength = (ti->fieldInfo[fieldNo].type.length == 65535);
			ti->fieldInfo[fieldNo].offset = bt->recordLength; 
			bt->recordLength+=ti->fieldInfo[fieldNo].type.length;
			if (ti->fieldInfo[fieldNo].type.length == 65535) {
				isLengthVarying=1;
			}
			if (ti->fieldInfo[fieldNo].type.id & IPFIX_ENTERPRISE_TYPE) {
				/* check if there are 8 bytes for this field */
				if (record+8 > endOfSet) {
					msg(MSG_ERROR, "IpfixParser: Template record exceeds set boundary!");
					delete bt;
					return;
				}
				ti->fieldInfo[fieldNo].type.eid = ntohl(*(uint32_t*)((uint8_t*)record+4));
				record = (uint8_t*)((uint8_t*)record+8);
			} else {
				ti->fieldInfo[fieldNo].type.eid = 0;
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
		if((sourceId->protocol == IPFIX_protocolIdentifier_UDP) && (templateLivetime > 0))
			bt->expires = time(0) + templateLivetime;
		else
			bt->expires = 0;

		IpfixOptionsTemplateRecord* ipfixRecord = optionsTemplateRecordIM.getNewInstance();
		ipfixRecord->sourceID = sourceId;
		ipfixRecord->optionsTemplateInfo = ti;
		statTotalDataRecords++;
		push(ipfixRecord);
	}
}

/**
 * Processes an IPFIX DataTemplate set.
 * Called by processMessage
 */
void IpfixParser::processDataTemplateSet(boost::shared_ptr<IpfixRecord::SourceID> sourceId, boost::shared_array<uint8_t> message, IpfixSetHeader* set, uint8_t* endOfMessage) {
	uint8_t* endOfSet = (uint8_t*)set + ntohs(set->length);
	uint8_t* record = (uint8_t*)&set->data;

	/* check if set length lies within message boundaries */
	if (endOfSet > endOfMessage) {
		msg(MSG_ERROR, "IpfixParser: Data template set exceeds message boundary!");
		return;
	}

	/* Data template record are >= 4 byte, so we stop processing when fewer bytes are left */
	while (record + 4 <= endOfSet) {
		/* First cast to normal template header used for withdrawal message */
		IpfixTemplateHeader* th = (IpfixTemplateHeader*)record;
		record = (uint8_t*)&th->data;
		if (th->fieldCount == 0) {
			/* This is a Template withdrawal message */
			templateBuffer->destroyBufferedTemplate(sourceId, ntohs(th->templateId));
			continue;
		}

		IpfixDataTemplateHeader* dth = (IpfixDataTemplateHeader*)th;
		record = (uint8_t*)&dth->data;
		/* Non-withdrawal data template records are >= 8 byte */
		if (record > endOfSet) {
			DPRINTF("Strange long padding in data template");
			return; 
		}

		TemplateBuffer::BufferedTemplate* bt = new TemplateBuffer::BufferedTemplate;
		boost::shared_ptr<IpfixRecord::DataTemplateInfo> ti(new IpfixRecord::DataTemplateInfo);
		bt->sourceID = sourceId;
		bt->templateID = ntohs(dth->templateId);
		bt->recordLength = 0;
		bt->setID = ntohs(set->id);
		bt->dataTemplateInfo = ti;
		ti->userData = 0;
		ti->templateId = ntohs(dth->templateId);
		ti->preceding = ntohs(dth->precedingRule);
		ti->fieldCount = ntohs(dth->fieldCount);
		ti->dataCount = ntohs(dth->dataCount);
		ti->fieldInfo = (IpfixRecord::FieldInfo*)malloc(ti->fieldCount * sizeof(IpfixRecord::FieldInfo));
		int isLengthVarying = 0;
		uint16_t fieldNo;
		for (fieldNo = 0; fieldNo < ti->fieldCount; fieldNo++) {
			/* check if there are at least 4 bytes for this field */
			if (record+4 > endOfSet) {
				msg(MSG_ERROR, "IpfixParser: Data template record exceeds set boundary!");
				delete bt;
				return;
			}
			ti->fieldInfo[fieldNo].type.id = ntohs(*(uint16_t*)((uint8_t*)record+0));
			ti->fieldInfo[fieldNo].type.length = ntohs(*(uint16_t*)((uint8_t*)record+2));
			ti->fieldInfo[fieldNo].type.isVariableLength = (ti->fieldInfo[fieldNo].type.length == 65535);
			ti->fieldInfo[fieldNo].offset = bt->recordLength; 
			bt->recordLength+=ti->fieldInfo[fieldNo].type.length;
			if (ti->fieldInfo[fieldNo].type.length == 65535) {
				isLengthVarying=1;
			}
			if (ti->fieldInfo[fieldNo].type.id & IPFIX_ENTERPRISE_TYPE) {
				/* check if there are 8 bytes for this field */
				if (record+8 > endOfSet) {
					msg(MSG_ERROR, "IpfixParser: Data template record exceeds set boundary!");
					delete bt;
					return;
				}
				ti->fieldInfo[fieldNo].type.eid = ntohl(*(uint32_t*)((uint8_t*)record+4));
				record = (uint8_t*)((uint8_t*)record+8);
			} else {
				ti->fieldInfo[fieldNo].type.eid = 0;
				record = (uint8_t*)((uint8_t*)record+4);
			}
		}
		if (isLengthVarying) {
			bt->recordLength = 65535;
			for (fieldNo = 0; fieldNo < ti->fieldCount; fieldNo++) {
				ti->fieldInfo[fieldNo].offset = 0xFFFFFFFF;
			}
		}

		ti->dataInfo = (IpfixRecord::FieldInfo*)malloc(ti->dataCount * sizeof(IpfixRecord::FieldInfo));
		for (fieldNo = 0; fieldNo < ti->dataCount; fieldNo++) {
			/* check if there are at least 4 bytes for this field */
			if (record+4 > endOfSet) {
				msg(MSG_ERROR, "IpfixParser: Data template record exceeds set boundary!");
				delete bt;
				return;
			}
			ti->dataInfo[fieldNo].type.id = ntohs(*(uint16_t*)((uint8_t*)record+0));
			ti->dataInfo[fieldNo].type.length = ntohs(*(uint16_t*)((uint8_t*)record+2));
			if (ti->dataInfo[fieldNo].type.id & IPFIX_ENTERPRISE_TYPE) {
				/* check if there are 8 bytes for this field */
				if (record+8 >= endOfSet) {
					msg(MSG_ERROR, "IpfixParser: Data template record exceeds set boundary!");
					delete bt;
					return;
				}
				ti->dataInfo[fieldNo].type.eid = ntohl(*(uint32_t*)((uint8_t*)record+4));
				record = (uint8_t*)((uint8_t*)record+8);
			} else {
				ti->dataInfo[fieldNo].type.eid = 0;
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
					return;
				}
				/* This is a variable-length field, get length from first byte and advance offset */
				ti->dataInfo[fieldNo].type.length = *(uint8_t*)(record + dataLength);
				dataLength += 1;
				if (ti->dataInfo[fieldNo].type.length == 255) {
					/* check if there are 2 bytes for the length */
					if (record + dataLength + 2 > endOfSet) {
						msg(MSG_ERROR, "IpfixParser: Template record exceeds set boundary!");
						delete bt;
						return;
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
			return;
		}

		/* Copy fixed data block */
		ti->data = (IpfixRecord::Data*)malloc(dataLength*sizeof(IpfixRecord::Data));
		ti->dataLength = dataLength;
		memcpy(ti->data, record, dataLength*sizeof(IpfixRecord::Data));
		 
		/* Advance record to end of fixed data block, i.e. start of next template record */
		record += dataLength;

		templateBuffer->bufferTemplate(bt); 
		if((sourceId->protocol == IPFIX_protocolIdentifier_UDP) && (templateLivetime > 0))
			bt->expires = time(0) + templateLivetime;
		else
			bt->expires = 0;
		
		IpfixDataTemplateRecord* ipfixRecord = dataTemplateRecordIM.getNewInstance();
		ipfixRecord->sourceID = sourceId;
		ipfixRecord->dataTemplateInfo = ti;
		push(ipfixRecord);
	}
}

/**
 * Processes an IPFIX data set.
 * Called by processMessage
 */
uint32_t IpfixParser::processDataSet(boost::shared_ptr<IpfixRecord::SourceID> sourceId, boost::shared_array<uint8_t> message, IpfixSetHeader* set, uint8_t* endOfMessage) {
	TemplateBuffer::BufferedTemplate* bt = templateBuffer->getBufferedTemplate(sourceId, ntohs(set->id));
	uint32_t numberOfRecords = 0;

	if (bt == 0) {
		/* this error may come in rapid succession; I hope I don't regret it */
		if(sourceId->exporterAddress.len == 4) {
			msg(MSG_INFO, "Template %d from %s unknown to collecting process", 
				ntohs(set->id), IPToString(*reinterpret_cast<uint32_t*>(&sourceId->exporterAddress)).c_str());
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
	if ((bt->setID == IPFIX_SetId_Template) || (bt->setID == NetflowV9_SetId_Template)) {
#else
	if (bt->setID == IPFIX_SetId_Template) {
#endif

		boost::shared_ptr<IpfixRecord::TemplateInfo> ti = bt->templateInfo;
        
		if (bt->recordLength < 65535) {
			if (record + bt->recordLength > endOfSet) {
				DPRINTF("Got a Data Set that contained not a single full record\n");
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
				statTotalDataRecords++;
				push(ipfixRecord);
				record = record + bt->recordLength;
				numberOfRecords++;
			}
		} else {
			/* Variable-length record */
			/* We assume that each field is at least 1 byte */
			if (record + ti->fieldCount > endOfSet) {
				DPRINTF("Got a Data Set that contained not a single full record");
			}
			else while (record < endOfSet) {
				int recordLength=0;
				int fieldLength;
				int i;
				bool incomplete = false;
				ti = boost::shared_ptr<IpfixRecord::TemplateInfo>(new IpfixRecord::TemplateInfo(*bt->templateInfo.get()));
				for (i = 0; i < ti->fieldCount; i++) {
					if (!ti->fieldInfo[i].type.isVariableLength) {
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
					DPRINTF("original length %u, offset %u", ti->fieldInfo[i].type.length, ti->fieldInfo[i].offset);
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
				statTotalDataRecords++;
				push(ipfixRecord);
				record = record + recordLength;
				numberOfRecords++;
			}
		}
	} else if (bt->setID == IPFIX_SetId_OptionsTemplate) {

		boost::shared_ptr<IpfixRecord::OptionsTemplateInfo> ti = bt->optionsTemplateInfo;

		if (bt->recordLength < 65535) {
			if (record + bt->recordLength > endOfSet) {
				DPRINTF("Got a Data Set that contained not a single full record\n");
			}
			else
			/* We stop processing when no full record is left */
			while (record + bt->recordLength <= endOfSet) {
				IpfixOptionsRecord* ipfixRecord = optionsRecordIM.getNewInstance();
				ipfixRecord->sourceID = sourceId;
				ipfixRecord->optionsTemplateInfo = ti;
				ipfixRecord->dataLength = bt->recordLength;
				ipfixRecord->message = message;
				ipfixRecord->data = record;
				statTotalDataRecords++;
				push(ipfixRecord);
				record = record + bt->recordLength;
				numberOfRecords++;
			}
		} else {
			/* We assume that each field is at least 1 byte */
			if (record + ti->fieldCount + ti->scopeCount > endOfSet) {
				DPRINTF("Got a Data Set that contained not a single full record");
			}
			else while (record < endOfSet) {
				int recordLength=0;
				int fieldLength;
				int i;
				bool incomplete = false;
				ti = boost::shared_ptr<IpfixRecord::OptionsTemplateInfo>(new IpfixRecord::OptionsTemplateInfo(*bt->optionsTemplateInfo.get()));
				for (i = 0; i < ti->scopeCount; i++) {
					if (!ti->scopeInfo[i].type.isVariableLength) {
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
					ti->scopeInfo[i].offset = recordLength;
					ti->scopeInfo[i].type.length = fieldLength;
					recordLength += fieldLength;
				}

				/* final check if entire record is within set boundary */
				if (incomplete || (record + recordLength > endOfSet)) {
					DPRINTF("Incomplete variable length record");
					break;
				} 

				for (i = 0; i < ti->fieldCount; i++) {
					if (!ti->fieldInfo[i].type.isVariableLength) {
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
					ti->fieldInfo[i].offset = recordLength;
					ti->fieldInfo[i].type.length = fieldLength;
					recordLength += fieldLength;
				}

				/* final check if entire record is within set boundary */
				if (incomplete || (record + recordLength > endOfSet)) {
					DPRINTF("Incomplete variable length record");
					break;
				} 

				IpfixOptionsRecord* ipfixRecord = optionsRecordIM.getNewInstance();
				ipfixRecord->sourceID = sourceId;
				ipfixRecord->optionsTemplateInfo = ti;
				ipfixRecord->dataLength = recordLength;
				ipfixRecord->message = message;
				ipfixRecord->data = record;
				statTotalDataRecords++;
				push(ipfixRecord);
				record = record + recordLength;
				numberOfRecords++;
			}
		}
	} else if (bt->setID == IPFIX_SetId_DataTemplate) {

		boost::shared_ptr<IpfixRecord::DataTemplateInfo> ti = bt->dataTemplateInfo;

		if (bt->recordLength < 65535) {
			if (record + bt->recordLength > endOfSet) {
				DPRINTF("Got a Data Set that contained not a single full record\n");
			}
			else 
			/* We stop processing when no full record is left */
			while (record + bt->recordLength <= endOfSet) {
				IpfixDataDataRecord* ipfixRecord = dataDataRecordIM.getNewInstance();
				ipfixRecord->sourceID = sourceId;
				ipfixRecord->dataTemplateInfo = ti;
				ipfixRecord->dataLength = bt->recordLength;
				ipfixRecord->message = message;
				ipfixRecord->data = record;
				statTotalDataRecords++;
				push(ipfixRecord);
				record = record + bt->recordLength;
				numberOfRecords++;
			}
		} else {
			/* We assume that each field is at least 1 byte */
			if (record + ti->fieldCount > endOfSet) {
				DPRINTF("Got a Data Set that contained not a single full record");
			}
			else while (record < endOfSet) {
				int recordLength=0;
				int fieldLength;
				int i;
				bool incomplete = false;
				ti =  boost::shared_ptr<IpfixRecord::DataTemplateInfo>(new IpfixRecord::DataTemplateInfo(*bt->dataTemplateInfo.get()));
				for (i = 0; i < ti->fieldCount; i++) {
					if (!ti->fieldInfo[i].type.isVariableLength) {
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
					ti->fieldInfo[i].offset = recordLength;
					ti->fieldInfo[i].type.length = fieldLength;
					recordLength += fieldLength;
				}

				/* final check if entire record is within set boundary */
				if (incomplete || (record + recordLength > endOfSet)) {
					DPRINTF("Incomplete variable length record");
					break;
				} 

				IpfixDataDataRecord* ipfixRecord = dataDataRecordIM.getNewInstance();
				ipfixRecord->sourceID = sourceId;
				ipfixRecord->dataTemplateInfo = ti;
				ipfixRecord->dataLength = recordLength;
				ipfixRecord->message = message;
				ipfixRecord->data = record;
				statTotalDataRecords++;
				push(ipfixRecord);
				record = record + recordLength;
				numberOfRecords++;
			}
		}	
	} else {
	    msg(MSG_FATAL, "Data Set based on known but unhandled template type %d", bt->setID);
	}
	return numberOfRecords;
}

        
/**
 * Process a NetflowV9 Packet
 * @return 0 on success
 */
int IpfixParser::processNetflowV9Packet(boost::shared_array<uint8_t> message, uint16_t length, boost::shared_ptr<IpfixRecord::SourceID> sourceId) {
	NetflowV9Header* header = (NetflowV9Header*)message.get();

	/* pointer to first set */
	IpfixSetHeader* set = (IpfixSetHeader*)&header->data;

	/* pointer beyond message */
	uint8_t* endOfMessage = (uint8_t*)((uint8_t*)message.get() + length); 

	int i;

	sourceId->observationDomainId = ntohl(header->observationDomainId);

	for (i = 0; i < ntohs(header->setCount); i++) {
		/* check if there is space for a set header */
		if ((uint8_t*)(set) + 4 > endOfMessage) {
		        msg(MSG_ERROR, "IpfixParser: Invalide NetFlowV9 message - set header exceeds message boundary!");
			return -1;
		}
		/* check set length */
		if (ntohs(set->length) < 3) {
			msg(MSG_ERROR, "IpfixParser: Invalid set length %u, must be >= 4", ntohs(set->length));
			return -1;
		}

		if (ntohs(set->id) == NetflowV9_SetId_Template) {
			processTemplateSet(sourceId, message, set, endOfMessage);
		} else
			if (ntohs(set->id) >= IPFIX_SetId_Data_Start) {
				processDataSet(sourceId, message, set, endOfMessage);
			} else {
				msg(MSG_ERROR, "Unsupported Set ID - expected 0/256+, got %d", ntohs(set->id));
			}
		set = (IpfixSetHeader*)((uint8_t*)set + ntohs(set->length));
	}

	return 0;
}

/**
 * Process an IPFIX Packet
 * @return 0 on success
 */
int IpfixParser::processIpfixPacket(boost::shared_array<uint8_t> message, uint16_t length, boost::shared_ptr<IpfixRecord::SourceID> sourceId) {
	IpfixHeader* header = (IpfixHeader*)message.get();
    sourceId->observationDomainId = ntohl(header->observationDomainId);

	if (ntohs(header->length) != length) {
		msg(MSG_ERROR, "IpfixParser: Bad message length - packet length is  %#06x, header length field is %#06x\n", length, ntohs(header->length));
		return -1;
	}

	/* pointer to first set */
	IpfixSetHeader* set = (IpfixSetHeader*)&header->data;

	/* pointer beyond message */
	uint8_t* endOfMessage = (uint8_t*)((uint8_t*)message.get() + length); 

	uint16_t tmpid;
	uint32_t numberOfDataRecords;
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
			processDataTemplateSet(sourceId, message, set, endOfMessage);
			break;
		case IPFIX_SetId_Template:
			processTemplateSet(sourceId, message, set, endOfMessage);
			break;
		case IPFIX_SetId_OptionsTemplate:
			processOptionsTemplateSet(sourceId, message, set, endOfMessage);
			break;
		default:
			if(tmpid >= IPFIX_SetId_Data_Start) {
				statTotalDRPackets++;
				numberOfDataRecords += processDataSet(sourceId, message, set, endOfMessage);
			} else {
				msg(MSG_ERROR, "processIpfixPacket: Unsupported Set ID - expected 2/3/4/256+, got %d", tmpid);
			}
		}
		set = (IpfixSetHeader*)((uint8_t*)set + ntohs(set->length));
	}
	
	//FIXME: check for out-of-order messages and lost records
	msg(MSG_VDEBUG, "Message contained %u bytes, sequence number was %u", numberOfDataRecords, ntohl(header->sequenceNo));

	return 0;
}

/**
 * Process new Message
 * @return 0 on success
 */
int IpfixParser::processPacket(boost::shared_array<uint8_t> message, uint16_t length, boost::shared_ptr<IpfixRecord::SourceID> sourceId)
{
	pthread_mutex_lock(&mutex);
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
	: templateLivetime(DEFAULT_TEMPLATE_EXPIRE_SECS),
	  statTotalDataRecords(0),
  	  statTotalDRPackets(0),
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

}


/**
 * statistics function called by StatisticsManager
 */
std::string IpfixParser::getStatisticsXML(double interval)
{
	ostringstream oss;
	
	oss << "<totalDataRecords>" << statTotalDataRecords << "</totalDataRecords>";
	oss << "<totalDataRecordUDPPackets>" << statTotalDRPackets << "</totalDataRecordUDPPackets>";

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
}

void IpfixParser::onReconfiguration1()
{
	setTemplateDestroyed(true);
}

/**
 * event forwarded from IpfixCollector, called after reconfiguration
 */
void IpfixParser::postReconfiguration()
{
	setTemplateDestroyed(false);
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
 * sets 'destroyed' flag of buffered templates
 * @param destroyed flag of buffered templates
 */
void IpfixParser::setTemplateDestroyed(bool destroyed)
{
	TemplateBuffer::BufferedTemplate* bt = templateBuffer->getFirstBufferedTemplate();
			
	while (bt) {
		bt->templateInfo.get()->destroyed = destroyed;
	
		bt = bt->next;
	}
}
