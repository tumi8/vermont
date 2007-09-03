/*
 * IPFIX Database Reader/Writer
 * Copyright (C) 2006 Jürgen Abberger
 * Copyright (C) 2006 Lothar Braun <braunl@informatik.uni-tuebingen.de>
 * Copyright (C) 2007 Gerhard Muenz
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

#include "FlowSink.hpp"
#include "IpfixDbCommon.hpp"
#include "IpfixParser.hpp"
#include "ipfix.hpp"
#include "ipfixlolib/ipfixlolib.h"
#include <mysql.h>
#include <netinet/in.h>
#include <time.h>

#define EXPORTERID 0

/**
 * IpfixDbWriter powered the communication to the database server
 * also between the other structs
 */
class IpfixDbWriter : public FlowSink {
	public:
		IpfixDbWriter(const char* host, const char* db,
				const char* user, const char* pw,
				unsigned int port, uint16_t observationDomainId,
				int maxStatements);
		~IpfixDbWriter();

		int start();
		int stop();

		int onDataRecord(IpfixRecord::SourceID* sourceID, IpfixRecord::TemplateInfo* templateInfo, uint16_t length, IpfixRecord::Data* data);
		int onDataDataRecord(IpfixRecord::SourceID* sourceID, IpfixRecord::DataTemplateInfo* dataTemplateInfo, uint16_t length, IpfixRecord::Data* data);
		int onTemplate(IpfixRecord::SourceID* sourceID, IpfixRecord::TemplateInfo* templateInfo);
		int onOptionsTemplate(IpfixRecord::SourceID* sourceID, IpfixRecord::OptionsTemplateInfo* optionsTemplateInfo);
		int onDataTemplate(IpfixRecord::SourceID* sourceID, IpfixRecord::DataTemplateInfo* dataTemplateInfo);
		int onOptionsRecord(IpfixRecord::SourceID* sourceID, IpfixRecord::OptionsTemplateInfo* optionsTemplateInfo, uint16_t length, IpfixRecord::Data* data);

		IpfixRecord::SourceID srcId;              /**Exporter default SourceID */

	protected:
		static const int MAX_TABLE = 3; /**< count of buffered tablenames */ 
		static const int MAX_EXP_TABLE = 3; /**< Count of buffered exporters. Increase this value if you use more exporters in parallel */

		/**
		 * Struct stores for each BufEntry TableBuffer[maxTable]
		 *  start-, endtime and tablename for the different tables
		 */
		typedef struct {
			uint64_t startTableTime;
			uint64_t endTableTime;                          
			char TableName[TABLE_WIDTH];
		} BufEntry;

		/**
		 * Store for each expTable ExporterBuffer[maxExpTable]
		 * exporterID,srcID and expIP for the different exporters
		 */
		typedef struct {
			int Id;          /** Id entry of sourcID and expIP in the ExporterTable */
			uint32_t observationDomainId;  /** observationDomainId of  the exporter monitor */
			uint32_t  expIp; /** IP of the exporter */
		} ExpTable;

		/** 
		 * Cache which stores recently used existing half-hour tables and exporter table entries to 
		 * reduce/avoid unnecessary mysql lookups
		 */
		typedef struct {
			int countBuffTable;                      /**counter of buffered table names*/
			IpfixDbWriter::BufEntry tableBuffer[MAX_TABLE];         /**buffer to store struct BufEntry*/             
			int countExpTable;                       /**counter of buffered exporter*/
			IpfixDbWriter::ExpTable exporterBuffer[MAX_EXP_TABLE];  /**buffer to store struct expTable*/
		} TableCache;

		TableCache cache;

		/** 
		 * Buffer for insert statements
		 */
		typedef struct {
			int statemReceived;                /**counter of insert into statements*/
			char** statemBuffer;               /**buffer  of char pointers to store the insert statements*/
			int maxStatements;
			char** lockTables;		   /**tables to look*/
			int maxLocks;
		} StatementBuffer;

		StatementBuffer statements;
			
		int numberOfColumns;         /**number of columns, used to calculate length of sql statements*/

		const char* hostName;        /** Hostname*/
		const char* dbName;          /**Name of the database*/
		const char* userName;        /**Username (default: Standarduser) */
		const char* password ;       /** Password (default: none) */
		unsigned int portNum;        /** Portnumber (use default) */
		const char* socketName;      /** Socketname (use default) */
		unsigned int flags;          /** Connectionflags (none) */
		MYSQL* conn;                 /** pointer to connection handle */       

		int createDB();
		int createExporterTable();
		int createDBTable(const char* tablename);
		char* getInsertStatement(IpfixRecord::SourceID* sourceID, IpfixRecord::DataTemplateInfo* dataTemplateInfo, uint16_t length, IpfixRecord::Data* data, char** locks, int maxlocks);
		int writeToDb();
		int getExporterID(IpfixRecord::SourceID* sourceID);
                const char* getTableName(uint64_t flowstartsec);
	private:
		void addColumnEntry(char* sql, const char* insert, bool quoted, bool lastcolumn);
		void addColumnEntry(char* sql, uint64_t insert, bool quoted, bool lastcolumn);
};


#endif


#endif
