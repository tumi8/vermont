/*
 * IPFIX Database Reader/Writer
 * Copyright (C) 2006 Jürgen Abberger
 * Copyright (C) 2006 Lothar Braun <braunl@informatik.uni-tuebingen.de>
 * Copyright (C) 2007, 2008 Gerhard Muenz
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
#ifdef DB_SUPPORT_ENABLED

#ifndef IPFIXDBWRITER_H
#define IPFIXDBWRITER_H

#include "IpfixDbCommon.hpp"
#include "IpfixParser.hpp"
#include "ipfix.hpp"
#include "ipfixlolib/ipfixlolib.h"
#include <mysql.h>
#include <netinet/in.h>
#include <time.h>
#include <sstream>

#define EXPORTERID 0

/**
 * IpfixDbWriter powered the communication to the database server
 * also between the other structs
 */
class IpfixDbWriter 
	: public IpfixRecordDestination, public Module, public Source<NullEmitable*>
{
	public:
		IpfixDbWriter(const string& hostname, const string& dbname,
				const string& username, const string& password,
				unsigned port, uint32_t observationDomainId, unsigned maxStatements,
				const vector<string>& columns);
		~IpfixDbWriter();

		void onDataRecord(IpfixDataRecord* record);
		void onDataDataRecord(IpfixDataDataRecord* record);

		/**
		 * Struct to identify the relationship between columns names and 
		 * IPFIX_TYPEID, column type and default value
		 */
		struct Column {
			const char* columnName; 	/** column name */
			const char* columnType;		/** column data type in database */
			uint64_t defaultValue;       /** default value */
			uint16_t ipfixId; 		/** IPFIX_TYPEID */
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
		MYSQL* conn;                 /** pointer to connection handle */  
		bool dbError;			// db error flag

		int createDB();
		int setCurrentTable(time_t flowstartsec);
		string& getInsertString(string& row, time_t& flowstartsec, const IpfixRecord::SourceID& sourceID,
				IpfixRecord::DataTemplateInfo& dataTemplateInfo,uint16_t length, IpfixRecord::Data* data);
		int writeToDb();
		int getExporterID(const IpfixRecord::SourceID& sourceID);
		int connectToDB();
		void processDataDataRecord(const IpfixRecord::SourceID& sourceID, 
				IpfixRecord::DataTemplateInfo& dataTemplateInfo, uint16_t length, 
				IpfixRecord::Data* data);


		uint64_t getData(IpfixRecord::FieldInfo::Type type, IpfixRecord::Data* data);
		bool equalExporter(const IpfixRecord::SourceID& a, const IpfixRecord::SourceID& b);
};


#endif


#endif
