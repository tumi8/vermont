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
 */

#ifdef REDIS_SUPPORT_ENABLED

#ifndef IPFIX_DB_WRITER_REDIS_H_
#define IPFIX_DB_WRITER_REDIS_H_

#include "IpfixDbCommon.hpp"
#include "../IpfixRecordDestination.h"
#include "common/ipfixlolib/ipfix.h"
#include "common/ipfixlolib/ipfixlolib.h"
#include <iostream>
#include <iomanip>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <time.h>
#include <sstream>
#include <vector>
#include <hiredis/hiredis.h>

#define EXPORTERID 0

/**
 * IpfixFlowInspectorExporter powered the communication to the redis database server
 * also between the other structs
 */
class IpfixFlowInspectorExporter 
	: public IpfixRecordDestination, public Module, public Source<NullEmitable*>
{
	public:
		IpfixFlowInspectorExporter(const string& hostname, const string& database,
				unsigned port);
		~IpfixFlowInspectorExporter();

		void onDataRecord(IpfixDataRecord* record);

                /**
                 * Struct to identify the relationship between columns names and 
                 * IPFIX_TYPEID, column type and default value
                 */
                struct Column {
                        const char* columnName;         /** column name */
                        const char* columnType;         /** column data type in database */
                        uint64_t defaultValue;          /** default value */
                        InformationElement::IeId ipfixId; /** IPFIX_TYPEID */
                        InformationElement::IeEnterpriseNumber enterprise; /** enterprise number */
                };

	private:
		// database data
		std::string dbHost, dbName;
		unsigned dbPort;
		redisContext* context;
		bool dbError;

		std::list<std::string> bufferedObjects;
		
		int connectToDB();
		void processDataDataRecord(const IpfixRecord::SourceID& sourceID, 
				TemplateInfo& dataTemplateInfo, uint16_t length, 
				IpfixRecord::Data* data);
		int writeToDb();
		uint64_t getData(InformationElement::IeInfo type, IpfixRecord::Data* data);
		std::string getInsertObj(TemplateInfo& dataTemplateInfo,uint16_t length, IpfixRecord::Data* data);
		const static Column identify[];
};


#endif

#endif

