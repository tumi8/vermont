/*
 * IPFIX Database Reader/Writer Oracle Connector
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
 *
 */

/* Some constants that are common to IpfixDbWriter and IpfixDbReader */
#ifdef ORACLE_SUPPORT_ENABLED

#ifndef IPFIXDBWRITERORACLE_H_
#define IPFIXDBWRITERORACLE_H_

#include "IpfixDbCommon.hpp"
#include "IpfixRecordDestination.h"
#include "common/ipfixlolib/ipfix.h"
#include "common/ipfixlolib/ipfixlolib.h"
#include <occi.h>
#include <iostream>
#include <iomanip>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <time.h>
#include <sstream>

using namespace std;

#define EXPORTERID 0

/**
 * IpfixDbWriterOracle powered the communication to the oracle database server
 * also between the other structs
 */
class IpfixDbWriterOracle 
	: public IpfixRecordDestination, public Module, public Source<NullEmitable*>
{
	public:
		IpfixDbWriterOracle(const string& hostname, const string& dbname,
				const string& username, const string& password,
				unsigned port, uint32_t observationDomainId, unsigned maxStatements,
				const vector<string>& columns);
		~IpfixDbWriterOracle();

		void onDataRecord(IpfixDataRecord* record);

		/**
		 * Struct to identify the relationship between columns names and 
		 * IPFIX_TYPEID, column type and default value
		 */
		struct Column {
			const char* columnName; 	/** column name */
			const char* columnType;		/** column data type in database */
			uint64_t defaultValue;       	/** default value */
			InformationElement::IeId ipfixId; /** IPFIX_TYPEID */
			InformationElement::IeEnterpriseNumber enterprise; /** enterprise number */
		};

	private:
		static const unsigned MAX_EXPORTER = 10;    // maximum numbers of cached exporters

		/**
		 * Struct buffers start and end time and tablename for the different tables
		 */
		struct TableCacheEntry {
			time_t startTime; // smallest flow start second timestamp in the table
			time_t endTime;   // largest flow start second timestamp in the table
			string name;   // name of the table
		};

		/**
		 * Struct buffers ODID, IP address and row index of an exporter
		 */
		struct ExporterCacheEntry {
			IpfixRecord::SourceID sourceID;/** source id of the exporter */
			int id;                        /** Id entry of sourcID and expIP in the ExporterTable */
		};


		TableCacheEntry currentTable;				// current table in tableCache

		list<ExporterCacheEntry> exporterCache;		// cached tables names, key=observationDomainId
		ExporterCacheEntry* currentExporter;			// pointer to current exporter in exporterCache

		IpfixRecord::SourceID srcId;           			// default source ID

		ostringstream insertStatement;			// insert statement string
		int numberOfInserts;					// number of inserts in statement
		int maxInserts;						// maximum number of inserts per statement

		vector<Column> tableColumns;			// table columns
		string tableColumnsString;     			// table columns as string for INSERT statements
		string tableColumnsCreateString;  			// table columns as string for CREATE statements

		// database data
		string dbHost, dbName, dbUser, dbPassword;
		unsigned dbPort;
		oracle::occi::Environment *env;
		oracle::occi::Connection *con;
		bool dbError;			// db error flag

		int createDB();
		int setCurrentTable(time_t flowstartsec);
		string& getInsertString(string& row, time_t& flowstartsec, const IpfixRecord::SourceID& sourceID,
				TemplateInfo& dataTemplateInfo,uint16_t length, IpfixRecord::Data* data);
		int writeToDb();
		int getExporterID(const IpfixRecord::SourceID& sourceID);
		int connectToDB();
		int createExporterTable();
		void processDataDataRecord(const IpfixRecord::SourceID& sourceID, 
				TemplateInfo& dataTemplateInfo, uint16_t length, 
				IpfixRecord::Data* data);

		uint64_t getData(InformationElement::IeInfo type, IpfixRecord::Data* data);
		bool equalExporter(const IpfixRecord::SourceID& a, const IpfixRecord::SourceID& b);

		const static Column identify[];
};


#endif

#endif

