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
#include "IpfixRecordDestination.h"
#include "common/ipfixlolib/ipfix.h"
#include "common/ipfixlolib/ipfixlolib.h"
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

		IpfixRecord::SourceID srcId;              /**Exporter default SourceID */

	protected:
		static const uint32_t MAX_EXP_TABLE = 10; /**< Count of buffered exporters. Increase this value if you use more exporters in parallel */
		static const uint32_t MAX_USEDTABLES = 5; /**< Number of cached entries for used (and created tables) */
		static const uint64_t TABLE_INTERVAL = 1000*24*3600; /**< Interval, in which new tables should be created (milliseconds).*/


		/**
		 * Buffer for insert statements
		 */
		struct InsertBuffer {
			uint32_t curRows;           /** counter of rows to be inserted */
			uint32_t maxRows;			/** maximum number of insert rows in buffer */
			char* appendPtr;		/** pointer to sql which marks position where to insert new data */
			char* bodyPtr;			/** pointer to sql which marks position where prefix of SQL statement ends */
			char* sql;     			/** one large buffer to contain INSERT statement */
		};

		struct Table {
			string name;
			uint64_t timeStart;
			uint64_t timeEnd;
		};


		list<string> usedPartitions;	/**< list of partitions that were last used */

		/**
		 * Stores information about different exporters encountered in SourceID
		 */
		struct ExporterEntry {
			int Id;          /** Id entry of sourcID and expIP in the ExporterTable */
			uint32_t observationDomainId;  /** observationDomainId of  the exporter monitor */
			uint32_t  ip; /** IP of the exporter */
		};

		InsertBuffer insertBuffer;
		ExporterEntry exporterEntries[MAX_EXP_TABLE];
		uint32_t curExporterEntries;

		uint32_t numberOfColumns;         /**number of columns, used to calculate length of sql statements*/

		const char* hostName;        /** Hostname*/
		const char* dbName;          /**Name of the database*/
		const char* userName;        /**Username (default: Standarduser) */
		const char* password ;       /** Password (default: none) */
		unsigned int portNum;        /** Portnumber (use default) */
		const char* socketName;      /** Socketname (use default) */
		unsigned int flags;          /** Connectionflags (none) */
		PGconn* conn;                /** pointer to connection handle */
		int dbError;
		Table curTable;			/** table name for currently cached entries in insertBuffer */
		string tablePrefix;			/** prefix for all tables */

		int createExporterTable();
		bool createDBTable(const char* partitionname, uint64_t starttime, uint64_t endtime);
		void addColumnEntry(const char* insert, bool quoted, bool lastcolumn);
		void addColumnEntry(const uint64_t insert, bool quoted, bool lastcolumn);
		void fillInsertRow(IpfixRecord::SourceID* sourceID,
				TemplateInfo* dataTemplateInfo, uint16_t length, IpfixRecord::Data* data);
		bool writeToDb();
		int getExporterID(IpfixRecord::SourceID* sourceID);
        bool checkCurrentTable(uint64_t flowStart);
        bool setCurrentTable(uint64_t flowStart);
        string getTimeAsString(uint64_t milliseconds, const char* formatstring, bool addfraction, uint32_t microseconds = 0);
		bool checkRelationExists(const char* relname);
	private:
		void connectToDB();
		void processDataDataRecord(IpfixRecord::SourceID* sourceID,
				TemplateInfo* dataTemplateInfo, uint16_t length,
				IpfixRecord::Data* data);

		/***** Internal Functions ****************************************************/

		char* getTableNamDependTime(char* tablename,uint64_t flowstartsec);

		uint64_t getdata(InformationElement::IeInfo type, IpfixRecord::Data* data);
		uint64_t getIPFIXValue(InformationElement::IeInfo type, IpfixRecord::Data* data);
		uint32_t getdefaultIPFIXdata(int ipfixtype);

		uint32_t getipv4address(InformationElement::IeInfo type, IpfixRecord::Data* data);
		void extractNtp64(uint64_t& intdata, uint32_t& micros);


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
