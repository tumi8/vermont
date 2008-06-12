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

#include <stdexcept>
#include <string.h>
#include <stdlib.h>
#include "IpfixDbWriter.hpp"
#include "common/msg.h"

/***** Internal types ******************************************************/

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
	int defaultValue; /** when no IPFIX_TYPEID is stored in the record,
	 *  use defaultvalue to store in database
	 */
};

/***** Global Variables ******************************************************/
/**
 * 	is needed to determine "now" time and the time of flowstartsseconds
 */
struct tm* timeNow;

/**
 * maximum length of one item in a SQL statement
 */
const uint16_t MAX_COL_LENGTH = 22;

/**
 *	struct to identify column names with IPFIX_TYPEID an the dataType to store in database
 *	ExporterID is no IPFIX_TYPEID, its user specified
 *      Attention: order of entries is important!
 */
const uint16_t ID_FIRSTSWITCHED_IDX = 8;
const uint16_t ID_LASTSWITCHED_IDX = 9;
struct Column identify [] = {
	{	"dstIP", IPFIX_TYPEID_destinationIPv4Address, "INTEGER(10) UNSIGNED",0},
	{	"srcIP", IPFIX_TYPEID_sourceIPv4Address, "INTEGER(10) UNSIGNED", 0},
	{	"srcPort", IPFIX_TYPEID_sourceTransportPort, "SMALLINT(5) UNSIGNED", 0},
	{	"dstPort", IPFIX_TYPEID_destinationTransportPort, "SMALLINT(5) UNSIGNED",0},
	{	"proto",IPFIX_TYPEID_protocolIdentifier , "TINYINT(3) UNSIGNED", 0},
	{	"dstTos", IPFIX_TYPEID_classOfServiceIPv4, "TINYINT(3) UNSIGNED", 0},
	{	"bytes", IPFIX_TYPEID_octetDeltaCount, "BIGINT(20) UNSIGNED", 0},
	{	"pkts", IPFIX_TYPEID_packetDeltaCount, "BIGINT(20) UNSIGNED", 0},
	{	"firstSwitched", IPFIX_TYPEID_flowStartSeconds, "INTEGER(10) UNSIGNED", 0}, // default value is invalid/not used for this entry
	{	"lastSwitched", IPFIX_TYPEID_flowEndSeconds, "INTEGER(10) UNSIGNED", 0}, // default value is invalid/not used for this entry
	{	"firstSwitchedMillis", IPFIX_TYPEID_flowStartMilliSeconds, "SMALLINT(5) UNSIGNED", 0},
	{	"lastSwitchedMillis", IPFIX_TYPEID_flowEndMilliSeconds, "SMALLINT(5) UNSIGNED", 0},
	{	"tcpControlBits", IPFIX_TYPEID_tcpControlBits,  "SMALLINT(5) UNSIGNED", 0},
	{	"revbytes", IPFIX_ETYPEID_revOctetDeltaCount, "BIGINT(20) UNSIGNED", 0},
	{	"revpkts", IPFIX_ETYPEID_revPacketDeltaCount, "BIGINT(20) UNSIGNED", 0},
	{	"revFirstSwitched", IPFIX_ETYPEID_revFlowStartSeconds, "INTEGER(10) UNSIGNED", 0}, // default value is invalid/not used for this entry
	{	"revLastSwitched", IPFIX_ETYPEID_revFlowEndSeconds, "INTEGER(10) UNSIGNED", 0}, // default value is invalid/not used for this entry
	{	"revFirstSwitchedMillis", IPFIX_ETYPEID_revFlowStartMilliSeconds, "SMALLINT(5) UNSIGNED", 0},
	{	"revLastSwitchedMillis", IPFIX_ETYPEID_revFlowEndMilliSeconds, "SMALLINT(5) UNSIGNED", 0},
	{	"revTcpControlBits", IPFIX_ETYPEID_revTcpControlBits,  "SMALLINT(5) UNSIGNED", 0},
	{	"exporterID",EXPORTERID, "SMALLINT(5) UNSIGNED", 0},
	{	0} // last entry must be 0
};

/***** Internal Functions ****************************************************/

char* getTableNamDependTime(char* tablename,uint64_t flowstartsec);
uint64_t getTableStartTime(uint64_t flowstartsec);
uint64_t getTableEndTime(uint64_t StartTime);

uint64_t getdata(IpfixRecord::FieldInfo::Type type, IpfixRecord::Data* data);
uint64_t getIPFIXValue(IpfixRecord::FieldInfo::Type type, IpfixRecord::Data* data);
uint32_t getdefaultIPFIXdata(int ipfixtype);

uint32_t getipv4address(IpfixRecord::FieldInfo::Type type, IpfixRecord::Data* data);

/**
 * create the database given by the name dbnam->dbn
 */
int IpfixDbWriter::createDB()
{
	/**Is there a already a database with the same name - drop it*/
	/* gerhard: let's keep the database, we do not want to lose data
	 char dropDb[STARTLEN];
	 strcpy(dropDb, "DROP DATABASE IF EXISTS ");
	 strncat(dropDb, dbName,strlen(dbName)+1);
	 msg(MSG_ERROR, "%s", dropDb);
	 if(mysql_query(conn, dropDb) != 0 ) {
	 msg(MSG_FATAL,"Drop of exists database failed. Error: %s",
	 mysql_error(conn));
	 return 1;
	 } */

	/** make query string to create database**/
	char createDbStr[STARTLEN];
	strcpy(createDbStr,"CREATE DATABASE IF NOT EXISTS ");
	strncat(createDbStr,dbName,strlen(dbName));
	/**create database*/
	if(mysql_query(conn, createDbStr) != 0 ) {
		msg(MSG_FATAL,"IpfixDbWriter: Creation of database %s failed. Error: %s",
				dbName, mysql_error(conn));
		return 1;
	}
	else {
		msg(MSG_INFO,"Database %s created",dbName);
	}
	/** use database  with dbName**/
	if(mysql_select_db(conn, dbName) !=0) {
		msg(MSG_FATAL,"IpfixDbWriter: Database %s not selectable. Error: %s",
				dbName, mysql_error(conn));
		return 1;
	}
	else {
		msg(MSG_DEBUG,"Database %s selected", dbName);
	}
	return 0;
}

int IpfixDbWriter::createExporterTable()
{
	/**is there already a table with the same name - drop it */
	/* gerhard: let's keep the database, we do not want to lose data
	 char dropExporterTab[STARTLEN];
	 strcpy(dropExporterTab,"DROP TABLE IF EXISTS exporter");
	 if(mysql_query(conn, dropExporterTab) != 0) {
	 msg(MSG_FATAL,"Drop of exists exporter table failed. Error: %s",
	 mysql_error(conn));
	 return 1;
	 }
	 */
	/** create table exporter*/

	// TODO: make this less ugly
	char createExporterTab[STARTLEN+(3 * COL_WIDTH)];
	strcpy(createExporterTab,"CREATE TABLE IF NOT EXISTS exporter (id SMALLINT(5) NOT NULL AUTO_INCREMENT,sourceID INTEGER(10) UNSIGNED DEFAULT NULL,srcIP INTEGER(10) UNSIGNED DEFAULT NULL,PRIMARY KEY(id))");
	if(mysql_query(conn,createExporterTab) != 0) {
		msg(MSG_FATAL,"IpfixDbWriter: Creation of table Exporter failed. Error: %s",
				mysql_error(conn));
		return 1;
	}
	else {
		msg(MSG_DEBUG,"Exporter table created");
	}
	return 0;
}

/**
 * 	Create a table in the database
 */
int IpfixDbWriter::createDBTable(const char* tablename)
{
	uint32_t i;
	char createTableStr[STARTLEN+(numberOfColumns * COL_WIDTH)];
	strcpy(createTableStr , "CREATE TABLE IF NOT EXISTS ");
	strncat(createTableStr,tablename,strlen(tablename)+1);
	strncat(createTableStr," (",(2*sizeof(char))+1);
	/**collect the names for columns and the dataTypes for the table in a string*/
	for(i=0; i < numberOfColumns; i++) {
		strncat(createTableStr,identify[i].cname,strlen(identify[i].cname)+1);
		strncat(createTableStr," ",sizeof(char)+1);
		strncat(createTableStr,identify[i].dataType,strlen(identify[i].dataType)+1);
		if( i != numberOfColumns-1) {
			strncat(createTableStr,",",sizeof(char)+1);
		}
	}
	strncat(createTableStr,")",sizeof(char)+1);

	/**Is there a already a table with the same name - drop it*/
	/* gerhard: oh no, do not drop any flow tables during operation
	 char dropTable[STARTLEN];
	 strcpy(dropTable,"DROP TABLE IF EXISTS ");
	 strncat(dropTable, tablename,strlen(tablename)+1);
	 if(mysql_query(conn, dropTable) != 0) {
	 msg(MSG_FATAL,"Drop of exists %s table failed. Error: %s",tablename,
	 mysql_error(conn));
	 return 1;
	 }
	 */
	/** create table*/
	if(mysql_query(conn,createTableStr) != 0) {
		msg(MSG_FATAL,"IpfixDbWriter: Creation of table failed. Error: %s",
				mysql_error(conn));
		return 1;
	}
	else {
		msg(MSG_INFO, "Table %s created ",tablename);
	}
	return 0;
}

/**
 * save given elements of record to database
 */
void IpfixDbWriter::processDataDataRecord(IpfixRecord::SourceID* sourceID, 
		IpfixRecord::DataTemplateInfo* dataTemplateInfo, uint16_t length, 
		IpfixRecord::Data* data)
{
	DPRINTF("Processing data record");

	/** check if statement buffer is not full*/
	if(statements.statemBuffer[statements.maxStatements-1][0] != '\0') {
		THROWEXCEPTION("IpfixDbWriter: Statement buffer is full, this should never happen - drop record");
	}

	/** sourceid null ? use default*/
	/* overwrite sourceid if defined */
	if(srcId.observationDomainId != 0 || sourceID == NULL) {
		sourceID = &srcId;
	}

	// if statement counter lower as  max count, insert record in statement buffer
	if (statements.statemReceived < statements.maxStatements) {
		// make an sql insert statement from the record data
		getInsertStatement(statements.statemBuffer[statements.statemReceived],
						   sourceID, dataTemplateInfo, length, data, statements.lockTables, statements.maxLocks);
		// only insert statement if getInsertStatement produced one
		if (statements.statemBuffer[statements.statemReceived][0]!=0) {
			if (strlen(statements.statemBuffer[statements.statemReceived])>=STARTLEN+(numberOfColumns*INS_WIDTH))
				msg(MSG_INFO, "length is %d, expected was %d", strlen(statements.statemBuffer[statements.statemReceived]), STARTLEN+(numberOfColumns*INS_WIDTH));
			DPRINTF("Insert statement: %s\n", statements.statemBuffer[statements.statemReceived]);
			// statemBuffer is filled ->  insert in table
			if(statements.statemReceived == statements.maxStatements-1) {
				msg(MSG_INFO, "Writing buffered records to database");
				writeToDb();
			}
			else {
				statements.statemReceived++;
				msg(MSG_DEBUG, "Buffering record. Need %i more records before writing to database.", statements.maxStatements - statements.statemReceived);
			}
		}
	}
}

/**
 *	function receive the DataRecord or DataDataRecord when callback is started
 */
void IpfixDbWriter::onDataDataRecord(IpfixDataDataRecord* record)
{
	processDataDataRecord(record->sourceID.get(), record->dataTemplateInfo.get(), 
			record->dataLength, record->data);
	
	record->removeReference();
}

/**
 *	function receive the  when callback is started
 */
void IpfixDbWriter::onDataRecord(IpfixDataRecord* record)
{
	// convert templateInfo to dataTemplateInfo
	IpfixRecord::DataTemplateInfo dataTemplateInfo;
	dataTemplateInfo.templateId = 0;
	dataTemplateInfo.preceding = 0;
	dataTemplateInfo.freePointers = false; // don't free the given pointers, as they are taken from a different structure
	dataTemplateInfo.fieldCount = record->templateInfo->fieldCount; /**< number of regular fields */
	dataTemplateInfo.fieldInfo = record->templateInfo->fieldInfo; /**< array of FieldInfos describing each of these fields */
	dataTemplateInfo.dataCount = 0; /**< number of fixed-value fields */
	dataTemplateInfo.dataInfo = NULL; /**< array of FieldInfos describing each of these fields */
	dataTemplateInfo.data = NULL; /**< data start pointer for fixed-value fields */
	dataTemplateInfo.userData = record->templateInfo->userData; /**< pointer to a field that can be used by higher-level modules */

	processDataDataRecord(record->sourceID.get(), &dataTemplateInfo, record->dataLength, record->data);
	
	record->removeReference();
}

/**
 * adds an entry for an sql statement
 */
void IpfixDbWriter::addColumnEntry(char* sql, const char* insert, bool quoted, bool lastcolumn)
{
	if (quoted) strcat(sql, "'");
	strncat(sql, insert, MAX_COL_LENGTH);
	if (quoted) strcat(sql, "'");
	if (!lastcolumn) strcat(sql, ", ");
	else strcat(sql, ") ");
}

/**
 * adds an entry for an sql statement
 */
void IpfixDbWriter::addColumnEntry(char* sql, uint64_t insert, bool quoted, bool lastcolumn)
{
	char strdata[30];
	snprintf(strdata, ARRAY_SIZE(strdata), "%Lu", insert);
	addColumnEntry(sql, strdata, quoted, lastcolumn);
}

/**
 *	loop over the IpfixRecord::DataTemplateInfo (fieldinfo,datainfo) to get the IPFIX values to store in database
 *	The result is written into statemStr which must have sufficient space!
 */
void IpfixDbWriter::getInsertStatement(char* statemStr, IpfixRecord::SourceID* sourceID,
		IpfixRecord::DataTemplateInfo* dataTemplateInfo,uint16_t length, IpfixRecord::Data* data, char** locks, uint32_t maxlocks)
{
	uint32_t j, k;
	uint64_t intdata = 0;
	uint32_t flowstartsec = 0;

	/**begin query string for insert statement*/
	strcpy(statemStr,"INSERT INTO ");

	/**make string for the column names*/
	char ColNames[numberOfColumns * INS_WIDTH];
	strcpy(ColNames," (");

	/**make string for the values  given by the IPFIX_TYPEID stored in the record*/
	char ColValues[numberOfColumns * INS_WIDTH];
	strcpy(ColValues," VALUES (");

	/**loop over the columname and loop over the IPFIX_TYPEID of the record
	 to get the corresponding data to store and make insert statement*/
	for( j=0; identify[j].cname != 0; j++) {
		bool notfound = true;

		if (identify[j].ipfixId == EXPORTERID) {
			/**lookup exporter buffer to get exporterID from sourcID and expIp**/
			uint32_t expID = getExporterID(sourceID);
			intdata = expID;
			notfound = false;
		}
		else {
			// try to gather data required for the field
			if(dataTemplateInfo->fieldCount > 0) {
				// look inside the ipfix data packet
				for(k=0; k < dataTemplateInfo->fieldCount; k++) {
					if(dataTemplateInfo->fieldInfo[k].type.id == identify[j].ipfixId) {
						notfound = false;
						intdata = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset));
						DPRINTF("IpfixDbWriter::getRecData: really saw ipfix id %d in packet with intdata %llX, type %d, length %d and offset %X", identify[j].ipfixId, intdata, dataTemplateInfo->fieldInfo[k].type.id, dataTemplateInfo->fieldInfo[k].type.length, dataTemplateInfo->fieldInfo[k].offset);
					}
				}
			}
			if( dataTemplateInfo->dataCount > 0 && notfound) {
				// look in static data fields of template for data
				for(k=0; k < dataTemplateInfo->dataCount; k++) {
					if(dataTemplateInfo->dataInfo[k].type.id == identify[j].ipfixId) {
						notfound = false;
						intdata = getdata(dataTemplateInfo->dataInfo[k].type,(dataTemplateInfo->data+dataTemplateInfo->dataInfo[k].offset));
					}
				}
			}
			if(notfound) {
				// for some Ids, we have an alternative
				switch (identify[j].ipfixId) {
					case IPFIX_TYPEID_flowStartSeconds:
						// look for alternative (flowStartMilliSeconds/1000)
						if(dataTemplateInfo->fieldCount > 0) {
							for(k=0; k < dataTemplateInfo->fieldCount; k++) {
								if(dataTemplateInfo->fieldInfo[k].type.id == IPFIX_TYPEID_flowStartMilliSeconds) {
									intdata = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset)) / 1000;
									notfound = false;
									break;
								}
							}
						}
						break;
					case IPFIX_ETYPEID_revFlowStartSeconds:
						// look for alternative (revFlowStartMilliSeconds/1000)
						if(dataTemplateInfo->fieldCount > 0) {
							for(k=0; k < dataTemplateInfo->fieldCount; k++) {
								if(dataTemplateInfo->fieldInfo[k].type.id == IPFIX_ETYPEID_revFlowStartMilliSeconds) {
									intdata = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset)) / 1000;
									notfound = false;
									break;
								}
							}
						}
						break;
					case IPFIX_TYPEID_flowEndSeconds:
						// look for alternative (flowEndMilliSeconds/1000)
						if(dataTemplateInfo->fieldCount > 0) {
							for(k=0; k < dataTemplateInfo->fieldCount; k++) {
								if(dataTemplateInfo->fieldInfo[k].type.id == IPFIX_TYPEID_flowEndMilliSeconds) {
									intdata = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset)) / 1000;
									notfound = false;
									break;
								}
							}
						}
						break;
					case IPFIX_ETYPEID_revFlowEndSeconds:
						// look for alternative (revFlowEndMilliSeconds/1000)
						if(dataTemplateInfo->fieldCount > 0) {
							for(k=0; k < dataTemplateInfo->fieldCount; k++) {
								if(dataTemplateInfo->fieldInfo[k].type.id == IPFIX_ETYPEID_revFlowEndMilliSeconds) {
									intdata = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset)) / 1000;
									notfound = false;
									break;
								}
							}
						}
						break;
				}
				// if still not found, get default value
				if(notfound)
					intdata = getdefaultIPFIXdata(identify[j].ipfixId);
			}

			// we need extra treatment for timing related fields
			switch (identify[j].ipfixId) {
				case IPFIX_TYPEID_flowStartSeconds:
					if(intdata==0) {
						// if no flow start time is available, maybe this is is from a netflow from Cisco
						// then use flowStartSysUpTime as flow start time
						for(k=0; k < dataTemplateInfo->fieldCount; k++) {
							if(dataTemplateInfo->fieldInfo[k].type.id == IPFIX_TYPEID_flowStartSysUpTime) {
								intdata = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset));
							}
						}
					}
					// save time for table access
					if (flowstartsec==0) flowstartsec = intdata;
					break;
					
				case IPFIX_TYPEID_flowEndSeconds:
					if(intdata==0) {
						// if no flow end time is available, maybe this is is from a netflow from Cisco
						// then use flowEndSysUpTime as flow start time
						for(k=0; k < dataTemplateInfo->fieldCount; k++) {
							if(dataTemplateInfo->fieldInfo[k].type.id == IPFIX_TYPEID_flowEndSysUpTime) {
								intdata = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset));
							}
						}
					}
					break;

				case IPFIX_TYPEID_flowStartMilliSeconds:
					// if flowStartSeconds is not stored in one of the columns, but flowStartMilliSeconds is,
					// then we use flowStartMilliSeconds for table access
					// This is realized by storing this value only if flowStartSeconds has not yet been seen.
					// A later appearing flowStartSeconds will override this value.
					if (flowstartsec==0)
						flowstartsec = intdata/1000;
					// in the database the millisecond entry is counted from last second
					intdata %= 1000;
					break;


				case IPFIX_TYPEID_flowEndMilliSeconds:
				case IPFIX_ETYPEID_revFlowStartMilliSeconds:				
				case IPFIX_ETYPEID_revFlowEndMilliSeconds:
					// in the database the millisecond entry is counted from last second
					intdata %= 1000;
					break;
			}
		}
		
		DPRINTF("saw ipfix id %d in packet with intdata %llX", identify[j].ipfixId, intdata);

		addColumnEntry(ColNames, identify[j].cname, false, j==numberOfColumns-1);
		addColumnEntry(ColValues, intdata, true, j==numberOfColumns-1);
	}

	/**make whole query string for the insert statement*/
	char tablename[TABLE_WIDTH];
	DPRINTF("flowstartsec: %d", flowstartsec);
	const char* tablen = getTableName(flowstartsec);
	strcpy(tablename, tablen);
	/** Insert statement = INSERT INTO + tablename +  Columnsname + Values of record*/
	strcat(statemStr, tablename);
	strcat(statemStr, ColNames);
	strcat(statemStr, ColValues);

	/* insert table name into locks if necessary */
	for(j=0; j < maxlocks; j++) {
		if(locks[j][0] == '\0') {
			/* empty slot, i.e. no more table names. insert the current one */
			strcpy(locks[j], tablename);
			break;}
		else if(strncmp(tablename, locks[j], TABLE_WIDTH) == 0)
		/* found tablename */
		break;
	}
	
	if (flowstartsec == 0) {
		msg(MSG_ERROR, "IpfixDbWriter: Failed to get timing data from record. Will be saved in default table: %s", statemStr);
	}
}

/**
 *	Function writes the content of the statemBuffer to database
 *	statemBuffer consist of single insert statements
 */
int IpfixDbWriter::writeToDb()
{
	uint32_t i;

	char LockTables[STARTLEN + (TABLE_WIDTH * statements.maxLocks * 2)];

	strcpy(LockTables,"LOCK TABLES ");
	//Lock all tables to store the insert statements
	for(i=0; i < statements.maxLocks; i++) {
		if(statements.lockTables[i][0] != '\0') {
			strncat(LockTables, statements.lockTables[i], strlen(statements.lockTables[i])+1);
			strncat(LockTables," WRITE", 6);
			// delete table name
			statements.lockTables[i][0] = '\0';
		}
		if((i < statements.maxLocks -1) && (statements.lockTables[i+1][0] != '\0'))
		strncat(LockTables,",", 1);
	}

	if(mysql_query(conn, LockTables) != 0) {
		msg(MSG_ERROR,"IpfixDbWriter: Lock of table failed. Error: %s",
				mysql_error(conn));
		return 1;
	}
	// Write the insert statement to database
	for(i=0; i != statements.maxStatements; i++) {
		if(statements.statemBuffer[i][0] != '\0') {
			if(mysql_query(conn, statements.statemBuffer[i]) != 0) {
				msg(MSG_ERROR,"IpfixDbWriter: Insert of records failed. Error: %s",
						mysql_error(conn));
				return 1;
			}
			else {
				DPRINTF("Record inserted\n");
			}
			statements.statemBuffer[i][0] = '\0';
		}
	}

	char UnLockTable[STARTLEN] = "UNLOCK TABLES";
	if(mysql_query(conn, UnLockTable) != 0) {
		msg(MSG_ERROR,"IpfixDbWriter: Unlock of tables failed",
				mysql_error(conn));
		return 1;
	}
	statements.statemReceived = 0;
	msg(MSG_DEBUG,"Write to database is complete");
	return 0;
}

/**
 *	Returns the tablename of a record according flowstartsec
 */
const char* IpfixDbWriter::getTableName(uint64_t flowstartsec)
{
	uint32_t i;

#ifdef DEBUG
	DPRINTF("Content of table cache :\n");
	for(i = 0; i < MAX_TABLE; i++) {
		DPRINTF("TableStartTime : %Lu TableEndTime : %Lu TableName : %s\n",
				cache.tableBuffer[i].startTableTime, cache.tableBuffer[i].endTableTime,
				cache.tableBuffer[i].TableName);
	}
#endif
	/** Is  flowstartsec in intervall of tablecreationtime in cache ?*/
	for(i = 0; i < MAX_TABLE; i++) {
		/**Is flowstartsec between  the range of tablestarttime and tableendtime? */
		if(cache.tableBuffer[i].startTableTime <= flowstartsec &&
				flowstartsec < cache.tableBuffer[i].endTableTime) {
			DPRINTF("Table: %s is in table cache\n", cache.tableBuffer[i].TableName);
			return cache.tableBuffer[i].TableName;
		}
	}

	/* Tablename is not in table cache */
	char tabNam[TABLE_WIDTH];
	getTableNamDependTime(tabNam, flowstartsec);

	uint64_t startTime = getTableStartTime(flowstartsec);
	uint64_t endTime = getTableEndTime(startTime);

	cache.tableBuffer[cache.countBuffTable].startTableTime = startTime;
	cache.tableBuffer[cache.countBuffTable].endTableTime = endTime;
	strcpy(cache.tableBuffer[cache.countBuffTable].TableName, tabNam);

	/** createTable when not in buffer*/
	if(createDBTable(cache.tableBuffer[cache.countBuffTable].TableName) != 0) {
		DPRINTF("Struct bufentry clean up after failure\n");
		cache.tableBuffer[cache.countBuffTable].startTableTime = 0;
		cache.tableBuffer[cache.countBuffTable].endTableTime = 0;
		cache.tableBuffer[cache.countBuffTable].TableName[0] = '\0';
		return 0;
	}

	/** If end of tablebuffer reached ?  Begin from  the start (keep recently used) */
	if(cache.countBuffTable < MAX_TABLE-1) {
		cache.countBuffTable++;
		return cache.tableBuffer[cache.countBuffTable-1].TableName;
	}
	else {
		cache.countBuffTable = 0;
		return cache.tableBuffer[MAX_TABLE-1].TableName;
	}
}

/** 
 *	The tablename according to the time of the records when the flow is started
 *	The result is given by "h_YYYYMMDD_HH_0 || 1"
 *	0, when the recordtime of min is  0 <= min < 30, otherwise 1
 */
char* getTableNamDependTime(char* tablename, uint64_t flowstartsec)
{
	char strtmp[TABLE_WIDTH];
	/** according to flowstartsec make the tablename*/
	time_t t = flowstartsec;
	/**time in Coordinated Universal Time - UTC, it was formerly Greenwich Mean Time - GMT*/
	/** for use local time, change expression gmtime() to localtime()*/
	timeNow = gmtime(&t);
	strcpy(tablename,"h_");
	snprintf(strtmp, ARRAY_SIZE(strtmp), "%u",timeNow->tm_year+1900);
	strncat(tablename,strtmp,strlen(strtmp)+1);
	snprintf(strtmp, ARRAY_SIZE(strtmp), "%02u",timeNow->tm_mon+1);
	strncat(tablename,strtmp,strlen(strtmp)+1);
	snprintf(strtmp, ARRAY_SIZE(strtmp), "%02u",timeNow->tm_mday);
	strncat(tablename,strtmp,strlen(strtmp)+1);
	strncat(tablename,"_",sizeof(char)+1);
	snprintf(strtmp, ARRAY_SIZE(strtmp), "%02u",timeNow->tm_hour);
	strncat(tablename,strtmp,strlen(strtmp)+1);
	strncat(tablename,"_",sizeof(char)+1);
	snprintf(strtmp, ARRAY_SIZE(strtmp), "%u",timeNow->tm_min<30?0:1);
	strncat(tablename,strtmp,strlen(strtmp)+1);

	return tablename;
}

/**
 *	Calculates the time of the tables according flowstartsec
 *	It determines in which table the record must be store
 */
uint64_t getTableStartTime(uint64_t flowstartsec)
{
	uint64_t startTime;
	time_t t = flowstartsec;
	timeNow = localtime(&t);

	if(timeNow->tm_min < 30) {
		timeNow->tm_min = 0;
		timeNow->tm_sec = 0;
		startTime = mktime(timeNow);
		return startTime;
	}
	else {
		timeNow->tm_min = 30;
		timeNow->tm_sec = 0;
		startTime = mktime(timeNow);
		return startTime;
	}
	return 0;
}

/** 
 *	Tableendtime is the time that past since tablestarttime plus the time for the duration time
 *	for tables to store
 * 	1800 sec is equal for 30 min tables
 */
uint64_t getTableEndTime(uint64_t startTime)
{
	uint64_t endTime = startTime + 1800;
	return endTime;
}

/**
 *	Returns the exporterID 
 *  	For every different sourcID and expIp a unique ExporterID will be generated from the database
 * 	First lookup for the ExporterID in the exporterBuffer according sourceID and expIp, is there nothing
 *  	lookup in the ExporterTable, is there also nothing insert sourceID and expIp an return the generated
 *      ExporterID
 */
int IpfixDbWriter::getExporterID(IpfixRecord::SourceID* sourceID)
{
	uint32_t i;
	MYSQL_RES* dbResult;
	MYSQL_ROW dbRow;
	int exporterID = 0;

	char statementStr[EXPORTER_WIDTH];
	uint32_t expIp;
	
	expIp = *(uint32_t*)(sourceID->exporterAddress.ip);

#ifdef DEBUG
	DPRINTF("Content of exporterBuffer\n");
	for(i = 0; i < MAX_EXP_TABLE; i++) {
		DPRINTF("exporterID:%d	   observationDomainID:%u	   expIp:%u\n",
				cache.exporterBuffer[i].Id, cache.exporterBuffer[i].observationDomainId,
				cache.exporterBuffer[i].expIp);
	}
#endif
	/** Is the exporterID already in exporterBuffer? */
	for(i = 0; i < MAX_EXP_TABLE; i++) {
		if(cache.exporterBuffer[i].observationDomainId == sourceID->observationDomainId &&
				cache.exporterBuffer[i].expIp== expIp &&
				cache.exporterBuffer[i].Id > 0) {
			DPRINTF("Exporter sourceID/IP with ID %d is in the exporterBuffer\n",
					cache.exporterBuffer[i].Id);
			return cache.exporterBuffer[i].Id;
		}
	}

	// it is not: try to get it from the database

	sprintf(statementStr, "SELECT id FROM exporter WHERE sourceID=%u AND srcIp=%u", sourceID->observationDomainId, expIp);

	if(mysql_query(conn, statementStr) != 0) {
		msg(MSG_ERROR,"IpfixDbWriter: Select on exporter table failed. Error: %s",
				mysql_error(conn));
		return 0;// If a failure occurs, return exporterID = 0
	}

	dbResult = mysql_store_result(conn);
	/** is the exporterID in the exporter table ?*/
	if(( dbRow = mysql_fetch_row(dbResult))) {
		exporterID = atoi(dbRow[0]);
		mysql_free_result(dbResult);
		DPRINTF("ExporterID %d is in exporter table\n",exporterID);
		/**Write new exporter in the exporterBuffer*/
		cache.exporterBuffer[cache.countExpTable].Id = exporterID;
		cache.exporterBuffer[cache.countExpTable].observationDomainId = sourceID->observationDomainId;
		cache.exporterBuffer[cache.countExpTable].expIp = expIp;
	}
	else
	{
		mysql_free_result(dbResult);
		
		/**ExporterID is not in exporter table - insert expID and expIp and return the exporterID*/
		char LockExporter[STARTLEN] = "LOCK TABLES exporter WRITE";
		char UnLockExporter[STARTLEN] = "UNLOCK TABLES";
		sprintf(statementStr, "INSERT INTO exporter (ID,sourceID,srcIP) VALUES ('NULL','%u','%u')",
				sourceID->observationDomainId, expIp);

		if(mysql_query(conn, LockExporter) != 0) {
			msg(MSG_ERROR,"IpfixDbWriter: Lock of exporter table failed. Error: %s",
					mysql_error(conn));
			return 0;
		}

		if(mysql_query(conn, statementStr) != 0) {
			msg(MSG_ERROR,"IpfixDbWriter: Insert in exporter table failed. Error: %s",
					conn);
			/**Unlock the table when a failure occur*/
			if(mysql_query(conn, UnLockExporter) != 0) {
				msg(MSG_ERROR,"IpfixDbWriter: UnLock of exporter table failed. Error: %s",
						mysql_error(conn));
				return 0;
			}
			return 0;
		}

		exporterID = mysql_insert_id(conn);
		msg(MSG_INFO,"ExporterID %d inserted in exporter table", exporterID);
		/**Write new exporter in the exporterBuffer*/
		cache.exporterBuffer[cache.countExpTable].Id = exporterID;
		cache.exporterBuffer[cache.countExpTable].observationDomainId = sourceID->observationDomainId;
		cache.exporterBuffer[cache.countExpTable].expIp = expIp;

		if(mysql_query(conn, UnLockExporter) != 0) {
			msg(MSG_ERROR,"IpfixDbWriter: UnLock of exporter table failed. Error: %s",
					mysql_error(conn));
			return 0;
		}
	}

	if(cache.countExpTable < MAX_EXP_TABLE-1) {
		cache.countExpTable++;
	}
	else {
		cache.countExpTable = 0;
	}

	return exporterID;
}

/**
 *	Get data of the record is given by the IPFIX_TYPEID
 */
uint64_t getdata(IpfixRecord::FieldInfo::Type type, IpfixRecord::Data* data)
{
	if(type.id == IPFIX_TYPEID_sourceIPv4Address || type.id == IPFIX_TYPEID_destinationIPv4Address)
	return getipv4address(type, data);
	else
	return getIPFIXValue(type, data);
}
/**
 *	determine the ipv4address of the data record
 */
uint32_t getipv4address( IpfixRecord::FieldInfo::Type type, IpfixRecord::Data* data)
{

	if (type.length > 5) {
		DPRINTF("IPv4 Address with length %d unparseable\n", type.length);
		return 0;
	}

	if ((type.length == 5) && ( type.id == IPFIX_TYPEID_sourceIPv4Address || IPFIX_TYPEID_destinationIPv4Address )) /*&& (imask != 0)*/{
		DPRINTF("imask drop from ipaddress\n");
		type.length = 4;
	}

	if ((type.length < 5) &&( type.id == IPFIX_TYPEID_sourceIPv4Address || type.id == IPFIX_TYPEID_destinationIPv4Address)) /*&& (imask == 0)*/{
		return getIPFIXValue(type, data);
	}

	return 0;
}

/**
 *	get the IPFIX value 
 */
uint64_t getIPFIXValue(IpfixRecord::FieldInfo::Type type, IpfixRecord::Data* data)
{
	switch (type.length) {
		case 1:
		return (*(uint8_t*)data);
		case 2:
		return ntohs(*(uint16_t*)data);
		case 4:
		return ntohl(*(uint32_t*)data);
		case 8:
		return ntohll(*(uint64_t*)data);
		default:
		printf("Uint with length %d unparseable\n", type.length);
		return 0;
	}
}

/**
 *	if there no IPFIX_TYPEID in the given data record 
 * 	get the default value to store in the database columns
 */
uint32_t getdefaultIPFIXdata(int ipfixtype_id)
{
	int i;
	for( i=0; identify[i].cname != 0; i++) {
		if(ipfixtype_id == identify[i].ipfixId) {
			return identify[i].defaultValue;
		}
	}
	return 0;
}

/***** Exported Functions ****************************************************/

/**
 * Creates a new ipfixDbWriter. Do not forget to call @c startipfixDbWriter() to begin writing to Database
 * @return handle to use when calling @c destroyipfixDbWriter()
 */
IpfixDbWriter::IpfixDbWriter(const char* host, const char* db,
		const char* user, const char* pw,
		unsigned int port, uint16_t observationDomainId,
		int maxStatements)
{
	conn = mysql_init(0); /** get the mysl init handle*/
	if(conn == 0) {
		msg(MSG_FATAL,"IpfixDbWriter: Get MySQL connect handle failed. Error: %s",
				mysql_error(conn));
		goto out;
	}
	else {
		msg(MSG_DEBUG,"IpfixDbWriter got MySQL init handler");
	}
	/**Initialize structure members IpfixDbWriter*/
	hostName = host;
	dbName = db;
	userName = user;
	password = pw;
	portNum = port;
	socketName = 0;
	flags = 0;
	srcId.observationDomainId = observationDomainId;

	/**Initialize table cache*/
	cache.countBuffTable = 0;
	cache.countExpTable = 0;
	uint32_t i;
	for(i = 0; i < MAX_TABLE; i++) {
		cache.tableBuffer[i].startTableTime = 0;
		cache.tableBuffer[i].endTableTime = 0;
		cache.tableBuffer[i].TableName[0] = '\0';
	}
	for(i = 0; i < MAX_EXP_TABLE; i++) {
		cache.exporterBuffer[i].Id = 0;
		cache.exporterBuffer[i].observationDomainId = 0;
		cache.exporterBuffer[i].expIp = 0;
	}

	/**count columns*/
	numberOfColumns = 0;
	for(i=0; identify[i].cname!=0; i++)
	numberOfColumns++;

	/**Initialize structure members Statement*/
	statements.statemBuffer = new char*[maxStatements]; //(char**)malloc(sizeof(char**)*maxStatements);
	statements.maxStatements = maxStatements;
	statements.statemReceived = 0;
	for( i = 0; i != statements.maxStatements; i++) {
		statements.statemBuffer[i] = new char[STARTLEN+(numberOfColumns*INS_WIDTH)]; //(char*) malloc((STARTLEN+(numberOfColumns * INS_WIDTH)) * sizeof(char));
		statements.statemBuffer[i][0] = '\0';
	}
	statements.lockTables = new char*[maxStatements]; //(char**)malloc(sizeof(char**)*maxStatements);
	statements.maxLocks = maxStatements; // worst case: every entry in another table
	for( i = 0; i != statements.maxLocks; i++) {
		statements.lockTables[i] = new char[TABLE_WIDTH]; //(char*) malloc(TABLE_WIDTH * sizeof(char));
		statements.lockTables[i][0] = '\0';
	}

	/**Connect to Database*/
	if (!mysql_real_connect(conn, hostName, userName,
					password, 0, portNum, socketName, flags)) {
		msg(MSG_FATAL,"IpfixDbWriter: Connection to database failed. Error: %s",
				mysql_error(conn));
		goto out;
	}
	else {
		msg(MSG_DEBUG,"IpfixDbWriter succesfully connected to database");
	}
	/** create Database*/
	if (createDB()!=0)
	goto out;
	/**create table exporter*/
	if (createExporterTable()!=0)
	goto out;

	return;

	out:
	THROWEXCEPTION("IpfixDbWriter creation failed");
	return;
}

/**
 * Frees memory used by an ipfixDbWriter
 * @param ipfixDbWriter handle obtained by calling @c createipfixDbWriter()
 */
IpfixDbWriter::~IpfixDbWriter()
{
	uint32_t i;

	writeToDb();
	mysql_close(conn);

	for(i=0; i<statements.statemReceived; i++)
		delete[] statements.statemBuffer[i];

	delete[] statements.statemBuffer;
	delete[] statements.lockTables;
}

#endif
