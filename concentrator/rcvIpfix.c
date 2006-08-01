/** @file
 * IPFIX Collector module.
 *
 * The IPFIX Collector module receives messages from lower levels (see @c processMessage())
 * and parses the message into separate Templates, Options and Flows. It then
 * invokes the appropriate callback routine for each Template, Option and Flow received
 * (see the @c setTemplateCallback() and @c setDataRecordCallback() function groups).
 *
 * The Collector module supports higher-level modules by providing field types and offsets along 
 * with the raw data block of individual messages passed via the callback functions (see @c TemplateInfo)
 *
 */

/******************************************************************************

IPFIX Collector module
Copyright (C) 2004 Christoph Sommer
http://www.deltadevelopment.de/users/christoph/ipfix

FIXME: Basic support for NetflowV9 packets, templates and flow records
is provided. Will break when fed field types with type ID >= 0x8000.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

******************************************************************************/

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

/* for ntohll et al */
#include "ipfixlolib/ipfixlolib.h"

#include "rcvIpfix.h"
#include "templateBuffer.h"
#include "ipfix.h"

#include "msg.h"
/***** Defines ************************************************************/

#define SUPPORT_NETFLOWV9

/***** Constants ************************************************************/

#define NetflowV9_SetId_Template  0

/***** Macros ************************************************************/


/***** Data Types ************************************************************/

/**
 * IPFIX header helper.
 * Constitutes the first 16 bytes of every IPFIX Message
 */
typedef struct {
	uint16_t version;			/**< Expected to be 0x000a */
	uint16_t length; 
	uint32_t exportTime;
	uint32_t sequenceNo;
	uint32_t sourceId;
	byte   data;
} IpfixHeader;

/**
 * NetflowV9 header helper.
 * Constitutes the first bytes of every NetflowV9 Message
 */
typedef struct {
	uint16_t version;                 /**< Expected to be 0x0009 */
	uint16_t setCount;
	uint32_t uptime;
	uint32_t exportTime;
	uint32_t sequenceNo;
	uint32_t sourceId;
	byte   data;
} NetflowV9Header;

/**
 * IPFIX "Set" helper.
 * Constitutes the first bytes of every IPFIX Template Set, Options Template Set or Data Set
 */
typedef struct {
	uint16_t id;
	uint16_t length;
	byte data; 
} IpfixSetHeader;

/**
 * IPFIX "Template Set" helper.
 * Constitutes the first bytes of every IPFIX Template
 */
typedef struct {
	uint16_t templateId;
	uint16_t fieldCount;
	byte data;
} IpfixTemplateHeader;

/**
 * IPFIX "DataTemplate Set" helper.
 * Constitutes the first bytes of every IPFIX DataTemplate
 */
typedef struct {
	uint16_t templateId;
	uint16_t fieldCount;
	uint16_t dataCount;
	uint16_t precedingRule;
	byte data;
} IpfixDataTemplateHeader;

/**
 * IPFIX "Options Template Set" helper.
 * Constitutes the first bytes of every IPFIX Options Template
 */
typedef struct {
	uint16_t templateId;
	uint16_t fieldCount;
	uint16_t scopeCount; 
	byte data;
} IpfixOptionsTemplateHeader;

/***** Global Variables ******************************************************/


/***** Internal Functions ****************************************************/

static void processDataSet(IpfixParser* ipfixParser, SourceID sourceID, IpfixSetHeader* set);
static void processTemplateSet(IpfixParser* ipfixParser, SourceID sourceID, IpfixSetHeader* set);
static void processDataTemplateSet(IpfixParser* ipfixParser, SourceID sourceID, IpfixSetHeader* set);
static void processOptionsTemplateSet(IpfixParser* ipfixParser, SourceID sourceId, IpfixSetHeader* set);

/**
 * Processes an IPFIX template set.
 * Called by processMessage
 */
static void processTemplateSet(IpfixParser* ipfixParser, SourceID sourceId, IpfixSetHeader* set) {
	byte* endOfSet = (byte*)set + ntohs(set->length);
	byte* record = (byte*)&set->data;

	/* TemplateSets are >= 4 byte, so we stop processing when only 3 bytes are left */
	while (record < endOfSet - 3) {
		IpfixTemplateHeader* th = (IpfixTemplateHeader*)record;
		record = (byte*)&th->data;
		if (th->fieldCount == 0) {
			/* This is a Template withdrawal message */
			destroyBufferedTemplate(ipfixParser->templateBuffer, sourceId, ntohs(th->templateId));
			continue;
		}
		BufferedTemplate* bt = (BufferedTemplate*)malloc(sizeof(BufferedTemplate));
		TemplateInfo* ti = (TemplateInfo*)malloc(sizeof(TemplateInfo));
		bt->sourceID = sourceId;
		bt->templateID = ntohs(th->templateId);
		bt->recordLength = 0;
		bt->setID = ntohs(set->id);
		bt->templateInfo = ti;
		ti->userData = 0;
		ti->templateId = ntohs(th->templateId);
		ti->fieldCount = ntohs(th->fieldCount);
		ti->fieldInfo = (FieldInfo*)malloc(ti->fieldCount * sizeof(FieldInfo));
		int isLengthVarying = 0;
		uint16_t fieldNo;
		for (fieldNo = 0; fieldNo < ti->fieldCount; fieldNo++) {
			ti->fieldInfo[fieldNo].type.id = ntohs(*(uint16_t*)((byte*)record+0));
			ti->fieldInfo[fieldNo].type.length = ntohs(*(uint16_t*)((byte*)record+2));
			ti->fieldInfo[fieldNo].type.isVariableLength = (ti->fieldInfo[fieldNo].type.length == 65535);
			ti->fieldInfo[fieldNo].offset = bt->recordLength; bt->recordLength+=ti->fieldInfo[fieldNo].type.length;
			if (ti->fieldInfo[fieldNo].type.length == 65535) {
				isLengthVarying=1;
			}
			if (ti->fieldInfo[fieldNo].type.id & IPFIX_ENTERPRISE_TYPE) {
				ti->fieldInfo[fieldNo].type.eid = ntohl(*(uint32_t*)((byte*)record+4));
				record = (byte*)((byte*)record+8);
			} else {
				ti->fieldInfo[fieldNo].type.eid = 0;
				record = (byte*)((byte*)record+4);
			}
		}
		if (isLengthVarying) {
			bt->recordLength = 65535;
			for (fieldNo = 0; fieldNo < ti->fieldCount; fieldNo++) {
				ti->fieldInfo[fieldNo].offset = 65535;
			}
		}
        
		bufferTemplate(ipfixParser->templateBuffer, bt); 
		// FIXME: Template expiration disabled for debugging
		// bt->expires = time(0) + TEMPLATE_EXPIRE_SECS;

		int n;          
		for (n = 0; n < ipfixParser->callbackCount; n++) {
			CallbackInfo* ci = &ipfixParser->callbackInfo[n];
			if (ci->templateCallbackFunction) {
				ci->templateCallbackFunction(ci->handle, sourceId, ti);
			}
		}
	}
}

/**
 * Processes an IPFIX Options Template Set.
 * Called by processMessage
 */
static void processOptionsTemplateSet(IpfixParser* ipfixParser, SourceID sourceId, IpfixSetHeader* set) {
	byte* endOfSet = (byte*)set + ntohs(set->length);
	byte* record = (byte*)&set->data;

	/* OptionsTemplateSets are >= 4 byte, so we stop processing when only 3 bytes are left */
	while (record < endOfSet - 3) {
		IpfixOptionsTemplateHeader* th = (IpfixOptionsTemplateHeader*)record;
		record = (byte*)&th->data;
		if (th->fieldCount == 0) {
			/* This is a Template withdrawal message */
			destroyBufferedTemplate(ipfixParser->templateBuffer, sourceId, ntohs(th->templateId));
			continue;
		}
		BufferedTemplate* bt = (BufferedTemplate*)malloc(sizeof(BufferedTemplate));
		OptionsTemplateInfo* ti = (OptionsTemplateInfo*)malloc(sizeof(OptionsTemplateInfo));
		bt->sourceID = sourceId;
		bt->templateID = ntohs(th->templateId);
		bt->recordLength = 0;
		bt->setID = ntohs(set->id);
		bt->optionsTemplateInfo = ti;
		ti->userData = 0;
		ti->templateId = ntohs(th->templateId);
		ti->scopeCount = ntohs(th->scopeCount);
		ti->scopeInfo = (FieldInfo*)malloc(ti->scopeCount * sizeof(FieldInfo));
		ti->fieldCount = ntohs(th->fieldCount)-ntohs(th->scopeCount);
		ti->fieldInfo = (FieldInfo*)malloc(ti->fieldCount * sizeof(FieldInfo));
		int isLengthVarying = 0;
		uint16_t scopeNo;
		for (scopeNo = 0; scopeNo < ti->scopeCount; scopeNo++) {
			ti->scopeInfo[scopeNo].type.id = ntohs(*(uint16_t*)((byte*)record+0));
			ti->scopeInfo[scopeNo].type.length = ntohs(*(uint16_t*)((byte*)record+2));
			ti->scopeInfo[scopeNo].type.isVariableLength = (ti->scopeInfo[scopeNo].type.length == 65535);
			ti->scopeInfo[scopeNo].offset = bt->recordLength; bt->recordLength+=ti->scopeInfo[scopeNo].type.length;
			if (ti->scopeInfo[scopeNo].type.length == 65535) {
				isLengthVarying=1;
			}
			if (ti->scopeInfo[scopeNo].type.id & IPFIX_ENTERPRISE_TYPE) {
				ti->scopeInfo[scopeNo].type.eid = ntohl(*(uint32_t*)((byte*)record+4));
				record = (byte*)((byte*)record+8);
			} else {
				ti->fieldInfo[scopeNo].type.eid = 0;
				record = (byte*)((byte*)record+4);
			}
		}
		uint16_t fieldNo;
		for (fieldNo = 0; fieldNo < ti->fieldCount; fieldNo++) {
			ti->fieldInfo[fieldNo].type.id = ntohs(*(uint16_t*)((byte*)record+0));
			ti->fieldInfo[fieldNo].type.length = ntohs(*(uint16_t*)((byte*)record+2));
			ti->fieldInfo[fieldNo].type.isVariableLength = (ti->fieldInfo[fieldNo].type.length == 65535);
			ti->fieldInfo[fieldNo].offset = bt->recordLength; bt->recordLength+=ti->fieldInfo[fieldNo].type.length;
			if (ti->fieldInfo[fieldNo].type.length == 65535) {
				isLengthVarying=1;
			}
			if (ti->fieldInfo[fieldNo].type.id & IPFIX_ENTERPRISE_TYPE) {
				ti->fieldInfo[fieldNo].type.eid = ntohl(*(uint32_t*)((byte*)record+4));
				record = (byte*)((byte*)record+8);
			} else {
				ti->fieldInfo[fieldNo].type.eid = 0;
				record = (byte*)((byte*)record+4);
			}
		}
		if (isLengthVarying) {
			bt->recordLength = 65535;
			for (fieldNo = 0; fieldNo < ti->scopeCount; fieldNo++) {
				ti->scopeInfo[fieldNo].offset = 65535;
			}
			for (fieldNo = 0; fieldNo < ti->fieldCount; fieldNo++) {
				ti->fieldInfo[fieldNo].offset = 65535;
			}
		}
		bufferTemplate(ipfixParser->templateBuffer, bt); 
		// FIXME: Template expiration disabled for debugging
		// bt->expires = time(0) + TEMPLATE_EXPIRE_SECS;


		int n;
		for (n = 0; n < ipfixParser->callbackCount; n++) {
			CallbackInfo* ci = &ipfixParser->callbackInfo[n];
			if (ci->optionsTemplateCallbackFunction) {
				ci->optionsTemplateCallbackFunction(ci->handle, sourceId, ti);
			}
		}
	}
}

/**
 * Processes an IPFIX DataTemplate set.
 * Called by processMessage
 */
static void processDataTemplateSet(IpfixParser* ipfixParser, SourceID sourceId, IpfixSetHeader* set) {
	byte* endOfSet = (byte*)set + ntohs(set->length);
	byte* record = (byte*)&set->data;

	/* DataTemplateSets are >= 4 byte, so we stop processing when only 3 bytes are left */
	while (record < endOfSet - 3) {
		IpfixDataTemplateHeader* th = (IpfixDataTemplateHeader*)record;
		record = (byte*)&th->data;
		if (th->fieldCount == 0) {
			/* This is a Template withdrawal message */
			destroyBufferedTemplate(ipfixParser->templateBuffer, sourceId, ntohs(th->templateId));
			continue;
		}
		BufferedTemplate* bt = (BufferedTemplate*)malloc(sizeof(BufferedTemplate));
		DataTemplateInfo* ti = (DataTemplateInfo*)malloc(sizeof(DataTemplateInfo));
		bt->sourceID = sourceId;
		bt->templateID = ntohs(th->templateId);
		bt->recordLength = 0;
		bt->setID = ntohs(set->id);
		bt->dataTemplateInfo = ti;
		ti->userData = 0;
		ti->id = ntohs(th->templateId);
		ti->preceding = ntohs(th->precedingRule);
		ti->fieldCount = ntohs(th->fieldCount);
		ti->dataCount = ntohs(th->dataCount);
		ti->fieldInfo = (FieldInfo*)malloc(ti->fieldCount * sizeof(FieldInfo));
		int isLengthVarying = 0;
		uint16_t fieldNo;
		for (fieldNo = 0; fieldNo < ti->fieldCount; fieldNo++) {
			ti->fieldInfo[fieldNo].type.id = ntohs(*(uint16_t*)((byte*)record+0));
			ti->fieldInfo[fieldNo].type.length = ntohs(*(uint16_t*)((byte*)record+2));
			ti->fieldInfo[fieldNo].type.isVariableLength = (ti->fieldInfo[fieldNo].type.length == 65535);
			ti->fieldInfo[fieldNo].offset = bt->recordLength; bt->recordLength+=ti->fieldInfo[fieldNo].type.length;
			if (ti->fieldInfo[fieldNo].type.length == 65535) {
				isLengthVarying=1;
			}
			if (ti->fieldInfo[fieldNo].type.id & IPFIX_ENTERPRISE_TYPE) {
				ti->fieldInfo[fieldNo].type.eid = ntohl(*(uint32_t*)((byte*)record+4));
				record = (byte*)((byte*)record+8);
			} else {
				ti->fieldInfo[fieldNo].type.eid = 0;
				record = (byte*)((byte*)record+4);
			}
		}
		if (isLengthVarying) {
			bt->recordLength = 65535;
			for (fieldNo = 0; fieldNo < ti->fieldCount; fieldNo++) {
				ti->fieldInfo[fieldNo].offset = 65535;
			}
		}

		ti->dataInfo = (FieldInfo*)malloc(ti->fieldCount * sizeof(FieldInfo));
		for (fieldNo = 0; fieldNo < ti->dataCount; fieldNo++) {
			ti->dataInfo[fieldNo].type.id = ntohs(*(uint16_t*)((byte*)record+0));
			ti->dataInfo[fieldNo].type.length = ntohs(*(uint16_t*)((byte*)record+2));
			if (ti->dataInfo[fieldNo].type.id & IPFIX_ENTERPRISE_TYPE) {
				ti->dataInfo[fieldNo].type.eid = ntohl(*(uint32_t*)((byte*)record+4));
				record = (byte*)((byte*)record+8);
			} else {
				ti->dataInfo[fieldNo].type.eid = 0;
				record = (byte*)((byte*)record+4);
			}
		}

		/* done with reading dataInfo, @c record now points to the fixed data block */
		byte* dataStart = record;

		int dataLength = 0;
		for (fieldNo = 0; fieldNo < ti->dataCount; fieldNo++) {
			ti->dataInfo[fieldNo].offset = dataLength;
			if (ti->dataInfo[fieldNo].type.length == 65535) {
				/* This is a variable-length field, get length from first byte and advance offset */
				ti->dataInfo[fieldNo].type.length = *(uint8_t*)(dataStart + ti->dataInfo[fieldNo].offset);
				ti->dataInfo[fieldNo].offset += 1;
				if (ti->dataInfo[fieldNo].type.length == 255) {
					/* First byte did not suffice, length is stored in next two bytes. Advance offset */
					ti->dataInfo[fieldNo].type.length = *(uint16_t*)(dataStart + ti->dataInfo[fieldNo].offset);
					ti->dataInfo[fieldNo].offset += 2;
				}
			}
			dataLength += ti->dataInfo[fieldNo].type.length;
		}

		/* Copy fixed data block */
		ti->data = (byte*)malloc(dataLength);
		memcpy(ti->data,dataStart,dataLength);

		/* Advance record to end of fixed data block, i.e. start of next template*/
		record += dataLength;

		bufferTemplate(ipfixParser->templateBuffer, bt); 
		// FIXME: Template expiration disabled for debugging
		// bt->expires = time(0) + TEMPLATE_EXPIRE_SECS;

		int n;
		for (n = 0; n < ipfixParser->callbackCount; n++) {
			CallbackInfo* ci = &ipfixParser->callbackInfo[n];
			if (ci->dataTemplateCallbackFunction){
				ci->dataTemplateCallbackFunction(ci->handle, sourceId, ti);
			}
		}
	}
}

/**
 * Processes an IPFIX data set.
 * Called by processMessage
 */
static void processDataSet(IpfixParser* ipfixParser, SourceID sourceId, IpfixSetHeader* set) {
	BufferedTemplate* bt = getBufferedTemplate(ipfixParser->templateBuffer, sourceId, ntohs(set->id));

	if (bt == 0) {
		/* this error may come in rapid succession; I hope I don't regret it */
		msg(MSG_INFO, "Template %d unknown to collecting process", ntohs(set->id));
		return;
	}
        
#ifdef SUPPORT_NETFLOWV9
	if ((bt->setID == IPFIX_SetId_Template) || (bt->setID == NetflowV9_SetId_Template)) {
#else
	if (bt->setID == IPFIX_SetId_Template) {
#endif

		TemplateInfo* ti = bt->templateInfo;
        
		uint16_t length = ntohs(set->length)-((byte*)(&set->data)-(byte*)set);

		byte* record = &set->data;
        
		if (bt->recordLength < 65535) {
			byte* recordX = record+length;
        
			if (record >= recordX - (bt->recordLength - 1)) {
				DPRINTF("Got a Data Set that contained not a single full record\n");
			}

			/* We stop processing when no full record is left */
			while (record < recordX - (bt->recordLength - 1)) {
				int n;
				for (n = 0; n < ipfixParser->callbackCount; n++) {
					CallbackInfo* ci = &ipfixParser->callbackInfo[n];
					if (ci->dataRecordCallbackFunction) {
						ci->dataRecordCallbackFunction(ci->handle, sourceId, ti, bt->recordLength, record);
					}
				}
				record = record + bt->recordLength;
			}
		} else {
			byte* recordX = record+length;

			if (record >= recordX - 3) {
				DPRINTF("Got a Data Set that contained not a single full record");
			}

			/* We assume that all variable-length records are >= 4 byte, so we stop processing when only 3 bytes are left */
			while (record < recordX - 3) {
				int recordLength=0;
				int i;
				for (i = 0; i < ti->fieldCount; i++) {
					int fieldLength = 0;
					if (!ti->fieldInfo[i].type.isVariableLength) {
						fieldLength = ti->fieldInfo[i].type.length;
					} else {
						fieldLength = *(uint8_t*)(record + recordLength);
						recordLength += 1;
						if (fieldLength == 255) {
							fieldLength = ntohs(*(uint16_t*)(record + recordLength));
							recordLength += 2;
						}
					}
					ti->fieldInfo[i].offset = recordLength;
					ti->fieldInfo[i].type.length = fieldLength;
					recordLength += fieldLength;
				}
				int n;
				for (n = 0; n < ipfixParser->callbackCount; n++) {
					CallbackInfo* ci = &ipfixParser->callbackInfo[n];
					if (ci->dataRecordCallbackFunction) {
						ci->dataRecordCallbackFunction(ci->handle, sourceId, ti, recordLength, record);
					}
				}
				record = record + recordLength;
			}
		}
	} else {
		if (bt->setID == IPFIX_SetId_OptionsTemplate) {

			OptionsTemplateInfo* ti = bt->optionsTemplateInfo;

			uint16_t length = ntohs(set->length)-((byte*)(&set->data)-(byte*)set);
			byte* record = &set->data;

			if (bt->recordLength < 65535) {
				byte* recordX = record+length;
				while (record < recordX) {
					int n;
					for (n = 0; n < ipfixParser->callbackCount; n++) {
						CallbackInfo* ci = &ipfixParser->callbackInfo[n];
						if (ci->optionsRecordCallbackFunction) {
							ci->optionsRecordCallbackFunction(ci->handle, sourceId, ti, bt->recordLength, record);
						}
					}
					record = record + bt->recordLength;
				}
			} else {
				byte* recordX = record+length;
				while (record < recordX) {
					int recordLength=0;
					int i;
					for (i = 0; i < ti->scopeCount; i++) {
						int fieldLength = 0;
						if (ti->scopeInfo[i].type.isVariableLength) {
							fieldLength = ti->scopeInfo[i].type.length;
						} else {
							fieldLength = *(uint8_t*)(record + recordLength);
							recordLength += 1;
							if (fieldLength == 255) {
								fieldLength = *(uint16_t*)(record + recordLength);
								recordLength += 2;
							}
						}
						ti->scopeInfo[i].offset = recordLength;
						ti->scopeInfo[i].type.length = fieldLength;
						recordLength += fieldLength;
					}
					for (i = 0; i < ti->fieldCount; i++) {
						int fieldLength = 0;
						if (!ti->fieldInfo[i].type.isVariableLength) {
							fieldLength = ti->fieldInfo[i].type.length;
						} else {
							fieldLength = *(uint8_t*)(record + recordLength);
							recordLength += 1;
							if (fieldLength == 255) {
								fieldLength = *(uint16_t*)(record + recordLength);
								recordLength += 2;
							}
						}
						ti->fieldInfo[i].offset = recordLength;
						ti->fieldInfo[i].type.length = fieldLength;
						recordLength += fieldLength;
					}
					int n;
					for (n = 0; n < ipfixParser->callbackCount; n++) {
						CallbackInfo* ci = &ipfixParser->callbackInfo[n];
						if (ci->optionsRecordCallbackFunction)
							ci->optionsRecordCallbackFunction(ci->handle, sourceId, ti, recordLength, record);
					}

					record = record + recordLength;
				}
			}
		} else {
			if (bt->setID == IPFIX_SetId_DataTemplate) {
				DataTemplateInfo* ti = bt->dataTemplateInfo;

				uint16_t length = ntohs(set->length)-((byte*)(&set->data)-(byte*)set);
				byte* record = &set->data;

				if (bt->recordLength < 65535) {
					byte* recordX = record+length;
					while (record < recordX) {
						int n;
						for (n = 0; n < ipfixParser->callbackCount; n++) {
							CallbackInfo* ci = &ipfixParser->callbackInfo[n];
							if (ci->dataDataRecordCallbackFunction) {
								ci->dataDataRecordCallbackFunction(ci->handle, sourceId, ti, bt->recordLength, record);
							}
						}
						record = record + bt->recordLength;
					}
				} else {
					byte* recordX = record+length;
					while (record < recordX) {
						int recordLength=0;
						int i;
						for (i = 0; i < ti->fieldCount; i++) {
							int fieldLength = 0;
							if (!ti->fieldInfo[i].type.isVariableLength) {
								fieldLength = ti->fieldInfo[i].type.length;
							} else {
								fieldLength = *(uint8_t*)(record + recordLength);
								recordLength += 1;
								if (fieldLength == 255) {
									fieldLength = *(uint16_t*)(record + recordLength);
									recordLength += 2;
								}
							}
							ti->fieldInfo[i].offset = recordLength;
							ti->fieldInfo[i].type.length = fieldLength;
							recordLength += fieldLength;
						}
						int n;
						for (n = 0; n < ipfixParser->callbackCount; n++) {
							CallbackInfo* ci = &ipfixParser->callbackInfo[n];
							if (ci->dataDataRecordCallbackFunction) {
								ci->dataDataRecordCallbackFunction(ci->handle, sourceId, ti, recordLength, record);
							}
						}
						record = record + recordLength;
					}
				}	
			} else {
				msg(MSG_FATAL, "Data Set based on known but unhandled template type %d", bt->setID);
			}
		}
	}
}

        
/**
 * Process a NetflowV9 Packet
 * @return 0 on success
 */     
static int processNetflowV9Packet(IpfixParser* ipfixParser, byte* message, uint16_t length) {
	NetflowV9Header* header = (NetflowV9Header*)message;

	/* pointer to first set */
	IpfixSetHeader* set = (IpfixSetHeader*)&header->data;

	int i;
	for (i = 0; i < ntohs(header->setCount); i++) {
		if (ntohs(set->id) == NetflowV9_SetId_Template) {
			processTemplateSet(ipfixParser, ntohl(header->sourceId), set);
		} else
			if (ntohs(set->id) >= IPFIX_SetId_Data_Start) {
				processDataSet(ipfixParser, ntohl(header->sourceId), set);
			} else {
				msg(MSG_ERROR, "Unsupported Set ID - expected 0/256+, got %d", ntohs(set->id));
			}
		set = (IpfixSetHeader*)((byte*)set + ntohs(set->length));
	}

	return 0;
}

/**
 * Process an IPFIX Packet
 * @return 0 on success
 */     
static int processIpfixPacket(IpfixParser* ipfixParser, byte* message, uint16_t length) {
	IpfixHeader* header = (IpfixHeader*)message;

	if (ntohs(header->length) != length) {
		DPRINTF("Bad message length - expected %#06x, got %#06x\n", length, ntohs(header->length));
		return -1;
	}

	/* pointer to first set */
	IpfixSetHeader* set = (IpfixSetHeader*)&header->data;

	/* pointer beyond message */
	IpfixSetHeader* setX = (IpfixSetHeader*)((char*)message + length); 

	uint16_t tmpid;
	SourceID tmpsid;
	while(set < setX) {
		tmpid=ntohs(set->id);
		tmpsid=ntohl(header->sourceId);

		switch(tmpid) {
		case IPFIX_SetId_DataTemplate:
			processDataTemplateSet(ipfixParser, tmpsid, set);
			break;
		case IPFIX_SetId_Template:
			processTemplateSet(ipfixParser, tmpsid, set);
			break;
		case IPFIX_SetId_OptionsTemplate:
			processOptionsTemplateSet(ipfixParser, tmpsid, set);
			break;
		default:
			if(tmpid >= IPFIX_SetId_Data_Start) {
				processDataSet(ipfixParser, tmpsid, set);
			} else {
				msg(MSG_ERROR, "processIpfixPacket: Unsupported Set ID - expected 2/3/4/256+, got %d", tmpid);
			}
		}
		set = (IpfixSetHeader*)((byte*)set + ntohs(set->length));
	}

	return 0;
}

/**
 * Process new Message
 * @return 0 on success
 */     
static int processMessage(IpfixParser* ipfixParser, byte* message, uint16_t length) {
	IpfixHeader* header = (IpfixHeader*)message;
	if (ntohs(header->version) == 0x000a) {
		return processIpfixPacket(ipfixParser, message, length);
	}
#ifdef SUPPORT_NETFLOWV9
	if (ntohs(header->version) == 0x0009) {
		return processNetflowV9Packet(ipfixParser, message, length);
	}
	DPRINTF("Bad message version - expected 0x009 or 0x000a, got %#06x\n", ntohs(header->version));
	return -1;
#else
	DPRINTF("Bad message version - expected 0x000a, got %#06x\n", ntohs(header->version));
	return -1;
#endif
}

static void printIPv4(FieldType type, FieldData* data) {
	int octet1 = 0;
	int octet2 = 0;
	int octet3 = 0;
	int octet4 = 0;
	int imask = 0;
	if (type.length >= 1) octet1 = data[0];
	if (type.length >= 2) octet2 = data[1];
	if (type.length >= 3) octet3 = data[2];
	if (type.length >= 4) octet4 = data[3];
	if (type.length >= 5) imask = data[4];
	if (type.length > 5) {
		DPRINTF("IPv4 Address with length %d unparseable\n", type.length);
		return;
	}

	if ((type.length == 5) /*&& (imask != 0)*/) {
		printf("%d.%d.%d.%d/%d", octet1, octet2, octet3, octet4, 32-imask);
	}  else {
		printf("%d.%d.%d.%d", octet1, octet2, octet3, octet4);
	}
}

static void printPort(FieldType type, FieldData* data) {
	if (type.length == 0) {
		printf("zero-length Port");
		return;
	}
	if (type.length == 2) {
		int port = ((uint16_t)data[0] << 8)+data[1];
		printf("%d", port);
		return;
	}
	if ((type.length >= 4) && ((type.length % 4) == 0)) {
		int i;
		for (i = 0; i < type.length; i+=4) {
			int starti = ((uint16_t)data[i+0] << 8)+data[i+1];
			int endi = ((uint16_t)data[i+2] << 8)+data[i+3];
			if (i > 0) {
				printf(",");
			}
			if (starti != endi) {
				printf("%d:%d", starti, endi);
			} else {
				printf("%d", starti);
			}
		}
		return;
	}

	printf("Port with length %d unparseable", type.length);
}

void printProtocol(FieldType type, FieldData* data) {
	if (type.length != 1) {
		printf("Protocol with length %d unparseable", type.length);
		return;
	}
	switch (data[0]) {
	case IPFIX_protocolIdentifier_ICMP:
		printf("ICMP");
		return;
	case IPFIX_protocolIdentifier_TCP:
		printf("TCP");
		return;
	case IPFIX_protocolIdentifier_UDP: 
		printf("UDP");
		return;
	case IPFIX_protocolIdentifier_RAW: 
		printf("RAW");
		return;
	default:
		printf("unknownProtocol");
		return;
	}
}

static void printUint(FieldType type, FieldData* data) {
	switch (type.length) {
	case 1:
		printf("%hhu",*(uint8_t*)data);
		return;
	case 2:
		printf("%hu",ntohs(*(uint16_t*)data));
		return;
	case 4:
		printf("%u",ntohl(*(uint32_t*)data));
		return;
	case 8:
		printf("%Lu",ntohll(*(uint64_t*)data));
		return;
	default:
		msg(MSG_ERROR, "Uint with length %d unparseable", type.length);
		return;
	}
}


/***** Exported Functions ****************************************************/


/**
 * Prints a string representation of FieldData to stdout.
 */
void printFieldData(FieldType type, FieldData* pattern) {
	char* s;

	switch (type.id) {
	case IPFIX_TYPEID_protocolIdentifier:
		printf("protocolIdentifier:");
		printProtocol(type, pattern);
		break;
	case IPFIX_TYPEID_sourceIPv4Address:
		printf("sourceIPv4Address:");
		printIPv4(type, pattern);
		break;
	case IPFIX_TYPEID_destinationIPv4Address:
		printf("destinationIPv4Address:");
		printIPv4(type, pattern);
		break;                          
	case IPFIX_TYPEID_sourceTransportPort:
		printf("sourceTransportPort:");
		printPort(type, pattern);
		break;
	case IPFIX_TYPEID_destinationTransportPort:
		printf("destinationTransportPort:");
		printPort(type, pattern);
		break;
	default:
		s = typeid2string(type.id);
		if (s != NULL) {
			printf("%s:", s);
			printUint(type, pattern);
		} else {
			DPRINTF("Field with ID %d unparseable\n", type.id);
		}
		break;
	}
}

/**
 * Gets a Template's FieldInfo by field id.
 * @param ti Template to search in
 * @param type Field id and field eid to look for, length is ignored
 * @return NULL if not found
 */
FieldInfo* getTemplateFieldInfo(TemplateInfo* ti, FieldType* type) {
	int i;

	for (i = 0; i < ti->fieldCount; i++) {
		if ((ti->fieldInfo[i].type.id == type->id) && (ti->fieldInfo[i].type.eid == type->eid)) {
			return &ti->fieldInfo[i];
		}
	}

	return NULL;
}

/**
 * Gets a DataTemplate's FieldInfo by field id.
 * @param ti DataTemplate to search in
 * @param type Field id and field eid to look for, length is ignored
 * @return NULL if not found
 */
FieldInfo* getDataTemplateFieldInfo(DataTemplateInfo* ti, FieldType* type) {
	int i;

	for (i = 0; i < ti->fieldCount; i++) {
		if ((ti->fieldInfo[i].type.id == type->id) && (ti->fieldInfo[i].type.eid == type->eid)) {
			return &ti->fieldInfo[i];
		}
	}

	return NULL;
}

/**
 * Gets a DataTemplate's Data-FieldInfo by field id.
 * @param ti DataTemplate to search in
 * @param type Field id and field eid to look for, length is ignored
 * @return NULL if not found
 */
FieldInfo* getDataTemplateDataInfo(DataTemplateInfo* ti, FieldType* type) {
	int i;

	for (i = 0; i < ti->dataCount; i++) {
		if ((ti->dataInfo[i].type.id == type->id) && (ti->dataInfo[i].type.eid == type->eid)) {
			return &ti->dataInfo[i];
		}
	}

	return NULL;            
}
        

/**
 * Creates a new  @c IpfixParser.
 * @return handle to created instance
 */
IpfixParser* createIpfixParser() {
	IpfixParser* ipfixParser;

	if(!(ipfixParser=(IpfixParser*)malloc(sizeof(IpfixParser)))) {
		msg(MSG_FATAL, "Ran out of memory");
		goto out0;
	}

	ipfixParser->callbackInfo = NULL;
	ipfixParser->callbackCount = 0;

	if(!(ipfixParser->templateBuffer = createTemplateBuffer(ipfixParser))) {
		msg(MSG_FATAL, "Could not create template Buffer");
		goto out1;
	}

	return ipfixParser;

out1:
	free(ipfixParser);
out0:
	return NULL;
}

/**
 * Frees memory used by an IpfixParser.
 */
void destroyIpfixParser(IpfixParser* ipfixParser) {
	destroyTemplateBuffer(ipfixParser->templateBuffer);

	free(ipfixParser->callbackInfo);

	free(ipfixParser);
}

/**
 * Creates a new @c PacketProcessor.
 * @return handle to the created object
 */
IpfixPacketProcessor*  createIpfixPacketProcessor() {
	IpfixPacketProcessor* packetProcessor;

	if(!(packetProcessor=(IpfixPacketProcessor*)malloc(sizeof(IpfixPacketProcessor)))) {
		msg(MSG_FATAL,"Ran out of memory");
		goto out0;
	}

        if (pthread_mutex_init(&packetProcessor->mutex, NULL) != 0) {
		msg(MSG_FATAL, "Could not init mutex");
		goto out1;
	}

	packetProcessor->ipfixParser = NULL;
	packetProcessor->processPacketCallbackFunction = processMessage;

	return packetProcessor;
out1:
	free(packetProcessor);
out0:
	return NULL;
}


/**
 * Frees memory used by a @c PacketProcessor
 */
void destroyIpfixPacketProcessor(IpfixPacketProcessor* packetProcessor) {
	destroyIpfixParser(packetProcessor->ipfixParser);
	pthread_mutex_destroy(&packetProcessor->mutex);
	free(packetProcessor);
}

/**
 * Adds a set of callback functions to the list of functions to call when a new Message arrives
 * @param ipfixParser IpfixParser to set the callback function for
 * @param handles set of callback functions
 */
void addIpfixParserCallbacks(IpfixParser* ipfixParser, CallbackInfo handles) {
	int n = ++ipfixParser->callbackCount;
	ipfixParser->callbackInfo = (CallbackInfo*)realloc(ipfixParser->callbackInfo, n * sizeof(CallbackInfo));
	memcpy(&ipfixParser->callbackInfo[n-1], &handles, sizeof(CallbackInfo));
}

/** 
 * Assigns an IpfixParser to packetProcessor
 * @param packetProcessor PacketProcessor to assign the IpfixParser to.
 * @param ipfixParser Pointer to an ipfixParser object.
 */
void setIpfixParser(IpfixPacketProcessor* packetProcessor, IpfixParser* ipfixParser) {
	packetProcessor->ipfixParser = ipfixParser;
}

/**
 * Adds a PacketProcessor to the list of PacketProcessors
 * @param ipfixCollector Collector to assign the PacketProcessor to
 * @param packetProcessor handle of packetProcessor
 */
void addIpfixPacketProcessor(IpfixCollector* ipfixCollector, IpfixPacketProcessor* packetProcessor) {
	int i;
	int n = ++ipfixCollector->processorCount;
	ipfixCollector->packetProcessors = (IpfixPacketProcessor*)realloc(ipfixCollector->packetProcessors,
									  n*sizeof(IpfixPacketProcessor));
	memcpy(&ipfixCollector->packetProcessors[n-1], packetProcessor, sizeof(IpfixPacketProcessor));

	for (i = 0; i != ipfixCollector->receiverCount; ++i) {
		setPacketProcessors(ipfixCollector->ipfixReceivers[i], ipfixCollector->packetProcessors,
				    ipfixCollector->processorCount);
	}
}


/**
 * Initializes internal data.
 * Call onces before using any function in this module
 * @return 0 if call succeeded
 */
int initializeIpfixCollectors() {
	initializeIpfixReceivers();
	return 0;
}

/**
 * Destroys internal data.
 * Call once to tidy up. Do not use any function in this module afterwards
 * @return 0 if call succeeded
 */
int deinitializeIpfixCollectors() {
	deinitializeIpfixReceivers();
	return 0;
} 

/**
 * Creates a new IpfixCollector.
 * Call @c startIpfixCollector() to start receiving and processing messages.
 * @param rec_type Type of receiver (SCTP, UDP, ...)
 * @param port Port to listen on
 * @return handle for further interaction
 */
IpfixCollector* createIpfixCollector(Receiver_Type rec_type, int port) {
	IpfixCollector* ipfixCollector;

	if (!(ipfixCollector = (IpfixCollector*)malloc(sizeof(IpfixCollector)))) {
		msg(MSG_FATAL, "Ran out of memory");
		return NULL;
	}

	ipfixCollector->receiverCount = 0;
	ipfixCollector->ipfixReceivers = NULL;

	ipfixCollector->processorCount = 0;
	ipfixCollector->packetProcessors = NULL;

	return ipfixCollector;
}

/**
 * Frees memory used by a IpfixCollector.
 * @param ipfixCollector Handle returned by @c createIpfixCollector()
 */
void destroyIpfixCollector(IpfixCollector* ipfixCollector) {
	int i;

	for (i = 0; i != ipfixCollector->receiverCount; ++i) {
		destroyIpfixReceiver(ipfixCollector->ipfixReceivers[i]);
	}
	free(ipfixCollector->ipfixReceivers);

	for (i = 0; i != ipfixCollector->processorCount; ++i) {
		destroyIpfixPacketProcessor(&ipfixCollector->packetProcessors[i]);
	}
	free(ipfixCollector);
}

/**
 * Starts receiving and processing messages.
 * All sockets prepared by calls to createIpfixCollector() will start
 * receiving messages until stopIpfixCollector() is called.
 * @return 0 on success, non-zero on error
 */
int startIpfixCollector(IpfixCollector* ipfixCollector) {
	int err = 0;
	int i;
	for (i = 0; i != ipfixCollector->receiverCount; ++i) {
		err += startIpfixReceiver(ipfixCollector->ipfixReceivers[i]); 
	}
	return err;
}

/**
 * Stops processing messages.
 * No more messages will be processed until the next startIpfixCollector() call.
 * @return 0 on success, non-zero on error
 */
int stopIpfixCollector(IpfixCollector* ipfixCollector) {
	int err = 0;
	int i;
	for (i = 0; i != ipfixCollector->receiverCount; ++i) {
		err += stopIpfixReceiver(ipfixCollector->ipfixReceivers[i]);
	}
	return err;
}

/**
 * Adds a IpfixReceiver to the list of IpfixReceivers
 * @param ipfixCollector Collector to assign the IpfixReceiver to
 * @param ipfixReceiver handle of ipfixReceiver
 */
void addIpfixReceiver(IpfixCollector* ipfixCollector, IpfixReceiver* ipfixReceiver) {
	int n = ++ipfixCollector->receiverCount;
	ipfixCollector->ipfixReceivers = (IpfixReceiver**)realloc(ipfixCollector->ipfixReceivers,
								  n*sizeof(IpfixReceiver*));
	ipfixCollector->ipfixReceivers[n - 1] = ipfixReceiver;
}

