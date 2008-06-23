/*
 * IPFIX Database Reader/Writer
 * Copyright (C) 2006 Jürgen Abberger
 * Copyright (C) 2006 Lothar Braun <braunl@informatik.uni-tuebingen.de>
 * Copyright (C) 2007 Gerhard Muenz
 * Copyright (C) 2008 Tobias Limmer
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
#ifdef PG_SUPPORT_ENABLED

#ifndef IPFIXDBWRITERPG_H
#define IPFIXDBWRITERPG_H

#include "IpfixDbCommon.hpp"
#include "IpfixParser.hpp"
#include "ipfix.hpp"
#include "ipfixlolib/ipfixlolib.h"
#include <libpq-fe.h>
#include <netinet/in.h>
#include <time.h>

#define EXPORTERID 0

/**
 * IpfixDbWriterPg powered the communication to the database server
 * also between the other structs
 */
class IpfixDbWriterPg
	: public IpfixRecordDestination, public Module, public Source<NullEmitable*>
{
	public:
		IpfixDbWriterPg(const char* host, const char* db,
				const char* user, const char* pw,
				unsigned int port, uint16_t observationDomainId, // FIXME: observationDomainId
				int maxStatements);
		~IpfixDbWriterPg();

		void onDataRecord(IpfixDataRecord* record);
		void onDataDataRecord(IpfixDataDataRecord* record);

		IpfixRecord::SourceID srcId;              /**Exporter default SourceID */

	protected:
		static const uint32_t MAX_TABLE = 3; /**< count of buffered tablenames */ 
		static const uint32_t MAX_EXP_TABLE = 3; /**< Count of buffered exporters. Increase this value if you use more exporters in parallel */

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
			uint32_t countBuffTable;                      /**counter of buffered table names*/
			IpfixDbWriterPg::BufEntry tableBuffer[MAX_TABLE];         /**buffer to store struct BufEntry*/             
			uint32_t countExpTable;                       /**counter of buffered exporter*/
			IpfixDbWriterPg::ExpTable exporterBuffer[MAX_EXP_TABLE];  /**buffer to store struct expTable*/
		} TableCache;

		TableCache cache;

		/** 
		 * Buffer for insert statements
		 */
		typedef struct {
			uint32_t statemReceived;                /**counter of insert into statements*/
			char** statemBuffer;               /**buffer  of char pointers to store the insert statements*/
			uint32_t  maxStatements;
			char** lockTables;		   /**tables to look*/
			uint maxLocks;
		} StatementBuffer;

		StatementBuffer statements;
			
		uint32_t numberOfColumns;         /**number of columns, used to calculate length of sql statements*/

		const char* hostName;        /** Hostname*/
		const char* dbName;          /**Name of the database*/
		const char* userName;        /**Username (default: Standarduser) */
		const char* password ;       /** Password (default: none) */
		unsigned int portNum;        /** Portnumber (use default) */
		const char* socketName;      /** Socketname (use default) */
		unsigned int flags;          /** Connectionflags (none) */
		PGconn* conn;                 /** pointer to connection handle */  
		int dbError;
		string tableName;

		int createExporterTable();
		int createDBTable(const char* tablename);
		void getInsertStatement(char* statemStr, IpfixRecord::SourceID* sourceID, IpfixRecord::DataTemplateInfo* dataTemplateInfo, uint16_t length, IpfixRecord::Data* data, char** locks, uint32_t maxlocks);
		int writeToDb();
		int getExporterID(IpfixRecord::SourceID* sourceID);
                const char* getTableName(uint64_t flowstartsec);
	private:
		void connectToDB();
		void addColumnEntry(char* sql, const char* insert, bool quoted, bool lastcolumn);
		void addColumnEntry(char* sql, uint64_t insert, bool quoted, bool lastcolumn);
		void processDataDataRecord(IpfixRecord::SourceID* sourceID, 
				IpfixRecord::DataTemplateInfo* dataTemplateInfo, uint16_t length, 
				IpfixRecord::Data* data);
		
		/***** Internal Functions ****************************************************/

		char* getTableNamDependTime(char* tablename,uint64_t flowstartsec);
		uint64_t getTableStartTime(uint64_t flowstartsec);
		uint64_t getTableEndTime(uint64_t StartTime);
		
		uint64_t getdata(IpfixRecord::FieldInfo::Type type, IpfixRecord::Data* data);
		uint64_t getIPFIXValue(IpfixRecord::FieldInfo::Type type, IpfixRecord::Data* data);
		uint32_t getdefaultIPFIXdata(int ipfixtype);
		
		uint32_t getipv4address(IpfixRecord::FieldInfo::Type type, IpfixRecord::Data* data);
		
		
		/**
		 * Identify the depency between columns names and 
		 * IPFIX_TYPEID working with a char pointer array
		 * in this array there is also standing  the defaultvalue
		 * of the IPFIX_TYPEID and the datatype to store in database
		 */
		struct Column {
			const char* cname; /** column name */
			int ipfixId; /** IPFIX_TYPEID */
			const char* dataType; /** which datatype to store in database */			 
			/**
			 *  when no IPFIX_TYPEID is stored in the record,
			 *  use defaultvalue to store in database
			 */
			int defaultValue;
		};
		const static Column identify[]; 
};


#endif


#endif
