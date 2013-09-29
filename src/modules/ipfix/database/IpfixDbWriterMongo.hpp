/*
 * IPFIX Database Writer Mongo Connector
 * Copyright (C) 2011 Philipp Fehre <philipp.fehre@googlemail.com>
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
Mongo  *
 */

/* Some constants that are common to IpfixDbWriter and IpfixDbReader */
#ifdef MONGO_SUPPORT_ENABLED

#ifndef IPFIXDBWRITERMONGO_H_
#define IPFIXDBWRITERMONGO_H_

/* Mongo dbclient.h also defines and uses msg Macro */


#include "IpfixDbCommon.hpp"
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

#undef msg
#include "client/dbclient.h"
#ifdef MONGO_VERSION_2
#include "util/net/hostandport.h"
#else
#include "util/hostandport.h"
#endif
#define msg(lvl, fmt, args...) msg2(__LINE__, __FILE__, __PRETTY_FUNCTION__, __func__, lvl, fmt, ##args)

using namespace std;

#define EXPORTERID 0

/**
 * IpfixDbWriterMongo powered the communication to the mongo database server
 * also between the other structs
 */
class IpfixDbWriterMongo 
	: public IpfixRecordDestination, public Module, public Source<NullEmitable*>
{
	public:
		IpfixDbWriterMongo(const string& hostname, const string& database,
				const string& username, const string& password,
				unsigned port, uint32_t observationDomainId, uint16_t maxStatements,
				const vector<string>& properties, bool beautifyProperties, bool allProperties);
		~IpfixDbWriterMongo();

		void onDataRecord(IpfixDataRecord* record);

		/**
		 * Struct to identify the relationship between columns names and 
		 * IPFIX_TYPEID, column type and default value
		 */
		struct Property {
			const char* propertyName; 	/** column name */
			uint64_t defaultValue;       	/** default value */
			InformationElement::IeId ipfixId; /** IPFIX_TYPEID */
			InformationElement::IeEnterpriseNumber enterprise; /** enterprise number */
		};

	private:
		static const unsigned MAX_EXPORTER = 10;    // maximum numbers of cached exporters
  
		/**
		 * Struct buffers ODID, IP address and row index of an exporter
		 */
		struct ExporterCacheEntry {
			IpfixRecord::SourceID sourceID;/** source id of the exporter */
			int id;                        /** Id entry of sourcID and expIP in the ExporterTable */
		};


		list<ExporterCacheEntry> exporterCache;		// cached tables names, key=observationDomainId
		ExporterCacheEntry* currentExporter;			// pointer to current exporter in exporterCache

		IpfixRecord::SourceID srcId;           			// default source ID
		vector<mongo::BSONObj> bufferedObjects; // Bulk insert via BSONObj vector 
		int numberOfInserts;					// number of inserts in statement
		int maxInserts;						// maximum number of inserts per statement

		vector<Property> documentProperties;			// Properties of inserted objects 

		// database data
		string dbHost, dbName, dbUser, dbPassword, dbCollectionFlows, dbCollectionExporters, dbCollectionCounters;
		unsigned dbPort;
		mongo::DBClientConnection con;
		bool beautyProp, allProp;
		bool dbError;			// db error flag
		mongo::BSONObj getInsertObj(const IpfixRecord::SourceID& sourceID,
				TemplateInfo& dataTemplateInfo,uint16_t length, IpfixRecord::Data* data);
		int writeToDb();
		int getExporterID(const IpfixRecord::SourceID& sourceID);
		int connectToDB();
		void processDataDataRecord(const IpfixRecord::SourceID& sourceID, 
				TemplateInfo& dataTemplateInfo, uint16_t length, 
				IpfixRecord::Data* data);

		uint64_t getData(InformationElement::IeInfo type, IpfixRecord::Data* data);
		bool equalExporter(const IpfixRecord::SourceID& a, const IpfixRecord::SourceID& b);

		const static Property identify[];
};


#endif

#endif

