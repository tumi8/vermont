/*
 * IPFIX Database Writer Redis Connector
 * Copyright (C) 2012 Lothar Braun <braun@net.in.tum.de>
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

#ifdef REDIS_SUPPORT_ENABLED

#include <string.h>
#include <iomanip>
#include <stdlib.h>
#include <vector>
#include "IpfixFlowInspectorExporter.hpp"
#include "common/msg.h"
#include  <hiredis/hiredis.h>

/**
 * (re)connect to database
 */
int IpfixFlowInspectorExporter::connectToDB()
{
	dbError = true;
	
	// If a connection exists don't reconnect
	if (context) return 0;
  
	// Connect
	std::string err;
	context = redisConnect(dbHost.c_str(), dbPort);
	if (context->err) {
		msg(MSG_FATAL,"IpfixFlowInspectorExporter: Redis connect failed. Error: %s", context->errstr);
		redisFree(context);
		context = NULL;
		return 1;
	}

	msg(MSG_DEBUG,"IpfixFlowInspectorExporter: Connection to Redis successful");
	dbError = false;
	return 0;
}

/**
 * save record to database
 */
void IpfixFlowInspectorExporter::processDataDataRecord(const IpfixRecord::SourceID& sourceID,
		TemplateInfo& dataTemplateInfo, uint16_t length,
		IpfixRecord::Data* data)
{
	std::string json_string;
	msg(MSG_DEBUG, "IpfixFlowInspectorExporter: Processing data record");

	if (dbError) {
		msg(MSG_DEBUG, "IpfixFlowInspectorExporter: reconnecting to DB");
		connectToDB();
		if (dbError) return;
	}

	json_string = getInsertObj(dataTemplateInfo, length, data);

	bufferedObjects.push_back(json_string);
	
	writeToDb();
}

std::string IpfixFlowInspectorExporter::getInsertObj(TemplateInfo& dataTemplateInfo,uint16_t length, IpfixRecord::Data* data)
{
	std::stringstream sstream;
	const struct ipfix_identifier* identifier;
	uint64_t intdata;

	sstream << "{ ";

	/* Dump all elements to DB */
	if(dataTemplateInfo.fieldCount > 0) {
		// look in ipfix records
		for(int k=0; k < dataTemplateInfo.fieldCount; k++) {
			intdata = getData(dataTemplateInfo.fieldInfo[k].type,(data+dataTemplateInfo.fieldInfo[k].offset));
			DPRINTF("IpfixFlowInspectorExporter::getData: dumping from packet intdata %llX, type %d, length %d and offset %X",
				intdata, dataTemplateInfo.fieldInfo[k].type.id, dataTemplateInfo.fieldInfo[k].type.length,
				dataTemplateInfo.fieldInfo[k].offset);

			if (k != 0) {
				sstream << ",";
			}
			identifier = ipfix_id_lookup(dataTemplateInfo.fieldInfo[k].type.id, dataTemplateInfo.fieldInfo[k].type.enterprise);
			if (identifier) {
				// push regular IPFIX name into queue 
				sstream << "\"" << identifier->name << "\" : ";
			} else {
				sstream << dataTemplateInfo.fieldInfo[k].type.id;
			}

			sstream << static_cast<long long int>(intdata);
		}
	}
			
	if( dataTemplateInfo.dataCount > 0) {
		// look in static data fields of template for data
		for(int k=0; k < dataTemplateInfo.dataCount; k++) {
			intdata = getData(dataTemplateInfo.dataInfo[k].type,(dataTemplateInfo.data+dataTemplateInfo.dataInfo[k].offset));
			if (k != 0) {
				sstream << ",";
			}
			identifier = ipfix_id_lookup(dataTemplateInfo.dataInfo[k].type.id, dataTemplateInfo.dataInfo[k].type.enterprise);
			if (identifier) {
				// push regular IPFIX name into queue 
				sstream << "\"" << identifier->name << "\" : ";
			} else {
				sstream << dataTemplateInfo.dataInfo[k].type.id;
			}

			sstream << static_cast<long long int>(intdata);
		}
	}

	sstream << " }";

	return sstream.str();
}

/*
 * Write Objects to database
 */
int IpfixFlowInspectorExporter::writeToDb()
{
	while (!bufferedObjects.empty()) {
		std::string& elem = bufferedObjects.front();
		redisReply *reply;
		reply = (redisReply*)redisCommand(context, "RPUSH %s %s", dbName.c_str(), elem.c_str());
		if (!reply) {
			msg(MSG_ERROR, "IpfixFlowInspectorExporter: Error while writing to redis queue: %s", reply->str);
			freeReplyObject(reply);
		}
		bufferedObjects.pop_front();
	}
	return 0; 
}

/**
 *	Get data of the record is given by the IPFIX_TYPEID
 */
uint64_t IpfixFlowInspectorExporter::getData(InformationElement::IeInfo type, IpfixRecord::Data* data)
{
	// TODO: workout a proper modular interpreter
	if (type.id >= IPFIX_TYPEID_mplsTopLabelStackSection &&
	    type.id <= IPFIX_TYPEID_mplsLabelStackSection10) {
			// Properly decode MPLS Label, Experimental and S fields
			// TODO: Provide all separated fields
			uint32_t MPLSStruct = 0;
			uint32_t Label = 0, Exp = 0, S = 0;
			memcpy(&MPLSStruct, data, 3);
			Label = MPLSStruct & 0xfffff0;
			Exp = MPLSStruct & 0x00000e;
			S = MPLSStruct & 0x000001;
			return ntohl(Label);
		}
	
	switch (type.length) {
		case 1:
			return (*(uint8_t*)data);
		case 2:
			return ntohs(*(uint16_t*)data);
		case 4:
			return ntohl(*(uint32_t*)data);
		case 5:	// may occur in the case if IP address + mask
			return ntohl(*(uint32_t*)data);
		case 8:
			return ntohll(*(uint64_t*)data);
		default:
			// TODO: dynamic lenght octectArray fields
			printf("Got element %d with unparsable length(%d).\n", type.id, type.length);
			return 0;
	}
}

/***** Public Methods ****************************************************/

/**
 * called on Data Record arrival
 */
void IpfixFlowInspectorExporter::onDataRecord(IpfixDataRecord* record)
{
	// only treat non-Options Data Records (although we cannot be sure that there is a Flow inside)
	if((record->templateInfo->setId != TemplateInfo::NetflowTemplate)
		&& (record->templateInfo->setId != TemplateInfo::IpfixTemplate)
		&& (record->templateInfo->setId != TemplateInfo::IpfixDataTemplate)) {
		record->removeReference();
		return;
	}

	msg(MSG_DEBUG, "IpfixFlowInspectorExporter: Data record received will be passed for processing");
	processDataDataRecord(*record->sourceID.get(), *record->templateInfo.get(),
			record->dataLength, record->data);

	record->removeReference();
}

/**
 * Constructor
 */
IpfixFlowInspectorExporter::IpfixFlowInspectorExporter(const string& hostname, const string& database, unsigned port)
	: dbHost(hostname), dbName(database), dbPort(port)
{
	if(connectToDB() != 0)
		THROWEXCEPTION("IpfixFlowInspectorExporter creation failed");
}


/**
 * Destructor
 */
IpfixFlowInspectorExporter::~IpfixFlowInspectorExporter()
{
	writeToDb();
}



#endif /* MONGO_SUPPORT_ENABLED */
