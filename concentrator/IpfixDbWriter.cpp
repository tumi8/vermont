/*
 * IPFIX Database Reader/Writer
 * Copyright (C) 2006 Jürgen Abberger
 * Copyright (C) 2006 Lothar Braun <braunl@informatik.uni-tuebingen.de>
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
struct Column{
        const char* cname;    /** column name */
        int ipfixId;          /** IPFIX_TYPEID */
        const char* dataType; /** which datatype to store in database */
        int defaultValue;     /** when no IPFIX_TYPEID is stored in the record,
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
const uint16_t   MAX_COL_LENGTH = 22;

 /**
 *	struct to identify column names with IPFIX_TYPEID an the dataType to store in database
 *	ExporterID is no IPFIX_TYPEID, its user specified
 *      Attention: order of entries is important!
 */
const uint16_t  ID_FIRSTSWITCHED_IDX = 8;
const uint16_t  ID_LASTSWITCHED_IDX = 9;
struct Column identify [] = {
	{"dstIP", IPFIX_TYPEID_destinationIPv4Address, "INTEGER(10) UNSIGNED",0},
	{"srcIP", IPFIX_TYPEID_sourceIPv4Address, "INTEGER(10) UNSIGNED", 0},	
	{"srcPort", IPFIX_TYPEID_sourceTransportPort,  "SMALLINT(5) UNSIGNED", 0},	
	{"dstPort", IPFIX_TYPEID_destinationTransportPort,  "SMALLINT(5) UNSIGNED",0},
	{"proto",IPFIX_TYPEID_protocolIdentifier , "TINYINT(3) UNSIGNED", 0},
	{"dstTos", IPFIX_TYPEID_classOfServiceIPv4, "TINYINT(3) UNSIGNED", 0},
	{"bytes", IPFIX_TYPEID_octetDeltaCount,  "BIGINT(20) UNSIGNED", 0},
	{"pkts", IPFIX_TYPEID_packetDeltaCount, "BIGINT(20) UNSIGNED", 0},
	{"firstSwitched", IPFIX_TYPEID_flowStartSeconds,  "INTEGER(10) UNSIGNED", 0},   // default value is invalid/not used for this entry
	{"lastSwitched", IPFIX_TYPEID_flowEndSeconds,  "INTEGER(10) UNSIGNED", 0},      // default value is invalid/not used for this entry
	{"firstSwitchedMillis", IPFIX_TYPEID_flowStartMilliSeconds,  "SMALLINT(5) UNSIGNED", 0},
	{"lastSwitchedMillis", IPFIX_TYPEID_flowEndMilliSeconds,  "SMALLINT(5) UNSIGNED", 0},
	{"exporterID",EXPORTERID, "SMALLINT(5) UNSIGNED", 0},
	{ 0 }
} ;

	
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
	char  createDbStr[STARTLEN] ;
	strcpy(createDbStr,"CREATE DATABASE IF NOT EXISTS ");
	strncat(createDbStr,dbName,strlen(dbName)+1);
	/**create database*/
	if(mysql_query(conn, createDbStr) != 0 ) {
		msg(MSG_FATAL,"IpfixDbWriter: Creation of database %s failed. Error: %s",
		    dbName, mysql_error(conn));
		return 1;
	} else {
		msg(MSG_DEBUG,"Database %s created",dbName);
	}
	/** use database  with dbName**/	
	if(mysql_select_db(conn, dbName) !=0) {
		msg(MSG_FATAL,"IpfixDbWriter: Database %s not selectable. Error: %s",
		    dbName, mysql_error(conn));
		return 1;
	} else {
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
	} else {
		msg(MSG_DEBUG,"Exporter table created");
	}
	return 0;
}

/**
* 	Create the table of the database
*/
int IpfixDbWriter::createDBTable(Table* table, const char* tablename)
{
	int i;
	char createTableStr[STARTLEN+(table->countCol* COL_WIDTH)];
	strcpy(createTableStr , "CREATE TABLE IF NOT EXISTS ");
	strncat(createTableStr,tablename,strlen(tablename)+1);
	strncat(createTableStr," (",(2*sizeof(char))+1);
	/**collect the names for columns and the dataTypes for the table in a string*/
	for(i=0; i < table->countCol; i++) {
            strncat(createTableStr,identify[i].cname,strlen(identify[i].cname)+1);
            strncat(createTableStr," ",sizeof(char)+1);
            strncat(createTableStr,identify[i].dataType,strlen(identify[i].dataType)+1);
            if( i  != table->countCol-1) {
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
	} else {
		msg(MSG_DEBUG, "Table %s created ",tablename);
	}
	return 0;
}

/**
*	function receive the DataRecord or DataDataRecord when callback is started
*/
int  IpfixDbWriter::onDataDataRecord(IpfixRecord::SourceID* sourceID, IpfixRecord::DataTemplateInfo* dataTemplateInfo, uint16_t length, IpfixRecord::Data* data)
{
	Table *tabl = table;
	Statement* statemen = tabl->statement;

	DPRINTF("Processing data record\n");

	/** if the writeToDb process not ready - drop record*/
	if(statemen->statemBuffer[statemen->maxStatements-1] != NULL) {
		msg(MSG_ERROR,"IpfixDbWriter: Statement buffer is full, writing to DB in progress? - drop record");
		return 1;
	}
	
	/** sourceid null ? use default*/
	/* overwrite sourceid if defined */
	if(srcId.observationDomainId != 0 || sourceID == NULL) {
		sourceID = &srcId;
	}

	/** make a sql insert statement from the recors data */
	char* insertTableStr = getRecData(tabl, sourceID,
					  dataTemplateInfo, length, data);
	DPRINTF("Insert statement: %s\n",insertTableStr);	
	
	/** if statement counter lower as  max count of statement then insert record in statemenBuffer*/
	if(statemen->statemReceived < statemen->maxStatements) {	
		statemen->statemBuffer[statemen->statemReceived] = insertTableStr;
		/** statemBuffer is filled ->  insert in table*/	
		if(statemen->statemReceived == statemen->maxStatements-1) {
                        msg(MSG_INFO, "Writing buffered records to database");
			writeToDb(tabl, statemen);
		} else {
                        msg(MSG_INFO, "Buffering record. Need %i more records before writing to database.", statemen->maxStatements - statemen->statemReceived);
			statemen->statemReceived++;
		}
	}
	return 0;
}

/**
 *	function receive the  when callback is started
 */
int IpfixDbWriter::onDataRecord(IpfixRecord::SourceID* sourceID, IpfixRecord::TemplateInfo* templateInfo, uint16_t length, IpfixRecord::Data* data)
{
	IpfixRecord::DataTemplateInfo dataTemplateInfo;
	
	dataTemplateInfo.templateId = 0;
	dataTemplateInfo.preceding = 0;
        dataTemplateInfo.freePointers = false;   // don't free the given pointers, as they are taken from a different structure
	dataTemplateInfo.fieldCount = templateInfo->fieldCount;  /**< number of regular fields */
	dataTemplateInfo.fieldInfo = templateInfo->fieldInfo;   /**< array of FieldInfos describing each of these fields */
	dataTemplateInfo.dataCount = 0;   /**< number of fixed-value fields */
	dataTemplateInfo.dataInfo = NULL;    /**< array of FieldInfos describing each of these fields */
	dataTemplateInfo.data = NULL;        /**< data start pointer for fixed-value fields */
	dataTemplateInfo.userData = templateInfo->userData;    /**< pointer to a field that can be used by higher-level modules */
	
	DPRINTF("receiveRec calls receiveDataRec\n");	

	return onDataDataRecord(sourceID, &dataTemplateInfo, length, data);
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
        sprintf(strdata, "%Lu", insert);
        addColumnEntry(sql, strdata, quoted, lastcolumn);
}

/**
*	loop over the IpfixRecord::DataTemplateInfo (fieldinfo,datainfo) to get the IPFIX values to store in database
*/
char* IpfixDbWriter::getRecData(Table* table, IpfixRecord::SourceID* sourceID,
		 IpfixRecord::DataTemplateInfo* dataTemplateInfo,uint16_t length, IpfixRecord::Data* data)
{
	int j, k, n;
	uint64_t intdata = 0;
	uint32_t flowstartsec = 0;

	bool flowstartseconds_seen = false;
	bool flowendseconds_seen = false;

	/**begin query string for insert statement*/
	char insert[STARTLEN+(table->countCol * INS_WIDTH)];
	strcpy(insert,"INSERT INTO ");
	
	/**make string for the column names*/
	char ColNames[table->countCol * INS_WIDTH];
	strcpy(ColNames," (");
	
	/**make string for the values  given by the IPFIX_TYPEID stored in the record*/ 
	char ColValues[table->countCol * INS_WIDTH]; 
	strcpy(ColValues," VALUES (");	
	
	/**loop over the columname and loop over the IPFIX_TYPEID of the record
	   to get the corresponding */
	/**data to store and make insert statement*/
        for( j=0; identify[j].cname != 0; j++) { 
                bool notfound = true;
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
                        for(n=0; n < dataTemplateInfo->dataCount; n++) {
                                if(dataTemplateInfo->dataInfo[n].type.id == identify[j].ipfixId) {
                                        notfound = false;
                                        intdata = getdata(dataTemplateInfo->dataInfo[n].type,(dataTemplateInfo->data+dataTemplateInfo->dataInfo[n].offset));
                                }
                        }
                }
                if(notfound) {	
                        if(identify[j].ipfixId == EXPORTERID) {
                                /**lookup exporter buffer to get exporterID from sourcID and expIp**/
                                uint32_t expID = getExporterID(table, sourceID);
                                intdata = expID;
                        } else {
                                intdata = getdefaultIPFIXdata(identify[j].ipfixId);
                        }
                }


                // we need extra treatment for timing related fields
                DPRINTF("saw ipfix id %d in packet with intdata %llX", identify[j].ipfixId, intdata);

                switch (identify[j].ipfixId) {
                    case IPFIX_TYPEID_flowStartSeconds:
                        // save time for table access
                        if (intdata != 0) {
                            flowstartsec = intdata;
                            flowstartseconds_seen = true;
                        }
                        break;

                    case IPFIX_TYPEID_flowEndSeconds:
                        if (intdata != 0) {
                            flowendseconds_seen = true;
                        }
                        break;

                    case IPFIX_TYPEID_flowStartMilliSeconds:

                        // if there is no filled flowStartSeconds field, we need to write this one too
                        if (!flowstartseconds_seen) {
                            flowstartseconds_seen = true;
                            flowstartsec = intdata/1000;

                            addColumnEntry(ColNames, identify[ID_FIRSTSWITCHED_IDX].cname, false, false);
                            addColumnEntry(ColValues, flowstartsec, true, false);
                        }
                        // in the database the millisecond entry is counted from last second
                        intdata %= 1000;

                        break;

                    case IPFIX_TYPEID_flowEndMilliSeconds:

                        // if there is no filled flowEndSeconds field, we need to write this one too
                        if (!flowendseconds_seen) {
                            flowendseconds_seen = true;
                            addColumnEntry(ColNames, identify[ID_LASTSWITCHED_IDX].cname, false, false);
                            addColumnEntry(ColValues, intdata/1000, true, false);
                        }
                        // in the database the millisecond entry is counted from last second
                        intdata %= 1000;

                        break;
                }

                // an empty entry for flowStart/EndSeconds must not be inserted into the database
                if (intdata != 0 || 
                   (identify[j].ipfixId != IPFIX_TYPEID_flowStartSeconds && 
                    identify[j].ipfixId != IPFIX_TYPEID_flowEndSeconds))
                {
                    addColumnEntry(ColNames, identify[j].cname, false, j==table->countCol-1);
                    addColumnEntry(ColValues, intdata, true, j==table->countCol-1);
                }
        }

        if (!flowstartseconds_seen || !flowendseconds_seen) {
            THROWEXCEPTION("failed to extract timing data from template, aborting (are fields flowStart/EndSeconds or flowStart/EndMilliseconds included in the IPFIX packet?)");
        }

        if (flowstartsec == 0) {
            THROWEXCEPTION("failed to get timing data from ipfix packet. this is a critical error at the moment, as no valid table can be determined. Aborting");
        }
	
	/**make whole query string for the insert statement*/
	char tablename[TABLE_WIDTH] ;
        DPRINTF("flowstartsec: %d", flowstartsec);
	const char* tablen = getTableName(table, flowstartsec);
	strcpy(tablename, tablen);
	/** Insert statement = INSERT INTO + tablename +  Columnsname + Values of record*/
	strcat(insert, tablename);
	strcat(insert, ColNames);
	strcat(insert, ColValues);
	
	
	char* insertTableStr = (char*) malloc((strlen(insert)+1)*sizeof(char)); 
	strcpy(insertTableStr,insert);
		
	return insertTableStr;
}


/**
*	Function writes the content of the statemBuffer to database
*	statemBuffer consist of single insert statements
*/
int IpfixDbWriter::writeToDb(Table* table, Statement* statement)
{
	int i ;
	
	char LockTables[STARTLEN * MAX_TABLE] ; 
	strcpy(LockTables,"LOCK TABLES ");
	/**Look all tables in the buffer to store the insert statements*/
	for(i=0; i < MAX_TABLE; i++) {
		if(strcmp(table->tableBuffer[i].TableName,"NULL") != 0) {
			strncat(LockTables, table->tableBuffer[i].TableName,strlen(table->tableBuffer[i].TableName)+1);
			strncat(LockTables," WRITE",(6*sizeof(char))+1);
		}
		if(strcmp(table->tableBuffer[i+1].TableName,"NULL") != 0 && (i+1) < MAX_TABLE)
			strncat(LockTables,",",sizeof(char)+1);
	}
	if(mysql_query(conn, LockTables) != 0) {
		msg(MSG_ERROR,"IpfixDbWriter: Lock of table failed. Error: %s",
		    mysql_error(conn));
		return 1;		    
	}
	/**Write the insert statement to database*/
	for(i=0; i != statement->maxStatements; i++) {
		if(mysql_query(conn, statement->statemBuffer[i]) != 0) {
			msg(MSG_ERROR,"IpfixDbWriter: Insert of records failed",
			    mysql_error(conn));
			return 1;
		} else {
			DPRINTF("Record inserted\n");
		}
		free(statement->statemBuffer[i]);
		statement->statemBuffer[i] = NULL;
	}
	
	char UnLockTable[STARTLEN] = "UNLOCK TABLES";
	if(mysql_query(conn, UnLockTable) != 0) {
		msg(MSG_ERROR,"IpfixDbWriter: Unlock of tables failed",
		    mysql_error(conn));
		return 1;
	}
	statement->statemReceived = 0;
	msg(MSG_INFO,"Write to database is complete");
	return 0;
}

/**
*	Returns the tablename of a record according flowstartsec
*/
const char* IpfixDbWriter::getTableName(Table*  table , uint64_t flowstartsec)
{
	int i;
	
#ifdef DEBUG
	DPRINTF("Content of tableBuffer :\n");
	for(i = 0; i < MAX_TABLE; i++) {
	    DPRINTF("TableStartTime : %Lu TableEndTime : %Lu TableName : %s\n",
		    table->tableBuffer[i].startTableTime, table->tableBuffer[i].endTableTime,
		    table->tableBuffer[i].TableName);	
	}
#endif
	/** Is  flowstartsec in intervall of tablecreationtime in buffer ?*/
	for(i = 0; i < MAX_TABLE; i++) {
	    /**Is flowstartsec between  the range of tablecreattime and tablecreattime+30 min*/
	    if(table->tableBuffer[i].startTableTime <= flowstartsec &&
		    flowstartsec < table->tableBuffer[i].endTableTime) {
		DPRINTF("Table: %s is in tableBuffer\n",  table->tableBuffer[i].TableName);
		return table->tableBuffer[i].TableName;
	    }
	}

	/**Tablename is not in tableBuffer*/	
	char tabNam[TABLE_WIDTH];
	getTableNamDependTime(tabNam, flowstartsec);
	
	uint64_t startTime = getTableStartTime(flowstartsec);
	uint64_t endTime = getTableEndTime(startTime);

	table->tableBuffer[table->countBuffTable].startTableTime = startTime;
	table->tableBuffer[table->countBuffTable].endTableTime = endTime;
	strcpy(table->tableBuffer[table->countBuffTable].TableName, tabNam);

	/** createTable when not in buffer*/
	if(createDBTable(table, table->tableBuffer[table->countBuffTable].TableName) != 0) {	
		DPRINTF("Struct bufentry clean up after failure\n");
		table->tableBuffer[table->countBuffTable].startTableTime = 0;
		table->tableBuffer[table->countBuffTable].endTableTime = 0;
		strcpy(table->tableBuffer[table->countBuffTable].TableName,"NULL");
		return 0; 
	}
	/** If end of tablebuffer reached ?  Begin from  the start*/  
	if(table->countBuffTable < MAX_TABLE-1){
		table->countBuffTable++;
	} else {
		table->countBuffTable = 0;
	}
	/** countBuffTable is null, when the last entry of tablebuffer is reached*/
	if(table->countBuffTable == 0)
		return table->tableBuffer[MAX_TABLE-1].TableName;
	else 
		return table->tableBuffer[table->countBuffTable-1].TableName;
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
	time_t  t = flowstartsec;
	/**time in Coordinated Universal Time - UTC, it was formerly Greenwich Mean Time - GMT*/
	/** for use local time, change expression gmtime() to localtime()*/
	timeNow = gmtime(&t);
	strcpy(tablename,"h_");
	sprintf(strtmp,"%u",timeNow->tm_year+1900);
	strncat(tablename,strtmp,strlen(strtmp)+1);
	sprintf(strtmp,"%02u",timeNow->tm_mon+1);
	strncat(tablename,strtmp,strlen(strtmp)+1);
	sprintf(strtmp,"%02u",timeNow->tm_mday);
	strncat(tablename,strtmp,strlen(strtmp)+1);
	strncat(tablename,"_",sizeof(char)+1);
	sprintf(strtmp,"%02u",timeNow->tm_hour);
	strncat(tablename,strtmp,strlen(strtmp)+1);
	strncat(tablename,"_",sizeof(char)+1);
	sprintf(strtmp,"%u",timeNow->tm_min<30?0:1);
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
	time_t  t = flowstartsec;
	timeNow = localtime(&t);
	
	if(timeNow->tm_min < 30) {
		timeNow->tm_min = 0;
		timeNow->tm_sec = 0;
		startTime = mktime(timeNow);
		return startTime;
	} else {
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
int IpfixDbWriter::getExporterID(Table* table, IpfixRecord::SourceID* sourceID)
{
	int i;
        MYSQL_RES* dbResult;
        MYSQL_ROW dbRow;
        int exporterID = 0;

        char selectStr[70];
        char stringtmp[10];
        uint32_t expIp;

        expIp = *(uint32_t*)(sourceID->exporterAddress.ip); 

#ifdef DEBUG
	DPRINTF("Content of exporterBuffer\n");
	for(i = 0; i < MAX_EXP_TABLE; i++) {
		DPRINTF("exporterID:%d	   observationDomainID:%u	   expIp:%u\n",
		    table->exporterBuffer[i].Id, table->exporterBuffer[i].observationDomainId,
		    table->exporterBuffer[i].expIp);
	}
#endif
	/** Is the exporterID already in exporterBuffer? */
	for(i = 0; i < MAX_EXP_TABLE; i++) {
		if(table->exporterBuffer[i].observationDomainId == sourceID->observationDomainId &&
		   table->exporterBuffer[i].expIp== expIp  &&
		   table->exporterBuffer[i].Id > 0) {
			DPRINTF("Exporter sourceID/IP with ID %d is in the exporterBuffer\n",
			    table->exporterBuffer[i].Id);
			return table->exporterBuffer[i].Id;
		}
	}

        // it is not: try to get it from the database

        sprintf(selectStr, "%s", "SELECT id FROM exporter WHERE sourceID=");
	sprintf(stringtmp,"%u",sourceID->observationDomainId);
	strncat(selectStr, stringtmp,strlen(stringtmp)+1);
	strncat(selectStr," AND srcIP=",(11*sizeof(char))+1);
	sprintf(stringtmp,"%u",expIp);
	strncat(selectStr, stringtmp,strlen(stringtmp)+1);
		
	if(mysql_query(conn, selectStr) != 0) {
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
		table->exporterBuffer[table->countExpTable].Id = exporterID;
		table->exporterBuffer[table->countExpTable].observationDomainId = sourceID->observationDomainId;
		table->exporterBuffer[table->countExpTable].expIp = expIp;
		
		if(table->countExpTable < MAX_EXP_TABLE-1) {
			table->countExpTable++;
		} else {
			table->countExpTable = 0;					
		}
		return exporterID;
	} 
	
	/**ExporterID is not in exporter table - insert expID and expIp and return the exporterID*/
	char LockExporter[STARTLEN] = "LOCK TABLES exporter WRITE";
	char UnLockExporter[STARTLEN] = "UNLOCK TABLES";
	char insertStr[70] = "INSERT INTO exporter (ID,sourceID,srcIP) VALUES('NULL','";
	sprintf(stringtmp,"%u",sourceID->observationDomainId);
	strncat(insertStr,stringtmp,strlen(stringtmp)+1);
	strncat(insertStr,"','",(3*sizeof(char))+1);
	sprintf(stringtmp,"%u",expIp);
	strncat(insertStr, stringtmp,strlen(stringtmp)+1);
	strncat(insertStr,"')",(2*sizeof(char))+1);
	
	mysql_free_result(dbResult);
	if(mysql_query(conn, LockExporter) != 0) {
		msg(MSG_ERROR,"IpfixDbWriter: Lock of exporter table failed. Error: %s",
		    mysql_error(conn));
		return 0;
	}
	
	if(mysql_query(conn, insertStr) != 0) {
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
	table->exporterBuffer[table->countExpTable].Id = exporterID;
	table->exporterBuffer[table->countExpTable].observationDomainId = sourceID->observationDomainId;
	table->exporterBuffer[table->countExpTable].expIp = expIp;
	
	if(table->countExpTable < MAX_EXP_TABLE-1) {
		table->countExpTable++;
	} else {
		table->countExpTable = 0;					
	}
		
	if(mysql_query(conn, UnLockExporter) != 0) {
		msg(MSG_ERROR,"IpfixDbWriter: UnLock of exporter table failed. Error: %s",
		    mysql_error(conn));
		return 0;
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

	if ((type.length == 5) && ( type.id == IPFIX_TYPEID_sourceIPv4Address || IPFIX_TYPEID_destinationIPv4Address )) /*&& (imask != 0)*/ {
		DPRINTF("imask drop from ipaddress\n");
		type.length = 4;
	}
	
	if ((type.length < 5) &&( type.id == IPFIX_TYPEID_sourceIPv4Address || type.id == IPFIX_TYPEID_destinationIPv4Address)) /*&& (imask == 0)*/ {
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
			return  (*(uint8_t*)data);
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
		if(ipfixtype_id == identify[i].ipfixId)	{
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
IpfixDbWriter::IpfixDbWriter(const char* hostName, const char* dbName,
                                   const char* userName, const char* password,
                                   unsigned int port, uint16_t observationDomainId,
                                   int maxStatements)
{	
	setSinkOwner("IpfixWriter");
	Table* tabl = (Table*)malloc(sizeof(Table));
	Statement* statemen = (Statement*)malloc(sizeof(Statement));
	statemen->statemBuffer = (char**)malloc(sizeof(char**)*maxStatements);
	statemen->maxStatements = maxStatements;
	
	this->conn = mysql_init(0);  /** get the mysl init handle*/
	if(this->conn == 0) {
		msg(MSG_FATAL,"IpfixDbWriter: Get MySQL connect handle failed. Error: %s",
		    mysql_error(this->conn));
		goto out;
	} else {
		msg(MSG_DEBUG,"IpfixDbWriter got MySQL init handler");
	}
	/**Initialize structure members IpfixDbWriter*/
	this->hostName = hostName;
	this->dbName = dbName;
	this->userName = userName;    		
	this->password = password;		
	this->portNum = port;
	this->socketName = 0;
	this->flags = 0;
	this->srcId.observationDomainId = observationDomainId;
	/**Initialize structure members Table*/	  
	this->table = tabl ;
	tabl->countBuffTable = 0;
	tabl->countExpTable = 0;
	
	/**Init tableBuffer start- , endTime and name of the tables*/
	int i ;
	for(i = 0; i < MAX_TABLE; i++) {
		tabl->tableBuffer[i].startTableTime = 0;
		tabl->tableBuffer[i].endTableTime  = 0;
		strcpy(tabl->tableBuffer[i].TableName, "NULL");
	}
	/**count columns*/
	tabl->countCol = 0;
	for(i=0; identify[i].cname!=0; i++)
		tabl->countCol++;
	
	/**Initialize structure members Statement*/	   	 	
	tabl->statement= statemen;
	statemen->statemReceived = 0;
	for( i = 0; i != statemen->maxStatements; i++) {
		statemen->statemBuffer[i] = NULL;
	}
	
	/**Init struct expTable*/
	for(i = 0; i < MAX_EXP_TABLE; i++) {
		tabl->exporterBuffer[i].Id = 0;
		tabl->exporterBuffer[i].observationDomainId = 0;
		tabl->exporterBuffer[i].expIp = 0;
	}				
	
	/**Connect to Database*/
	if (!mysql_real_connect(this->conn,
				this->hostName, this->userName,
				this->password, 0, this->portNum,
				this->socketName, this->flags)) {
		msg(MSG_FATAL,"IpfixDbWriter: Connection to database failed. Error: %s",
		    mysql_error(this->conn));
		goto out;
	} else {
		msg(MSG_DEBUG,"IpfixDbWriter succesfully connected to database");
	}
	/** create Database*/
	if(createDB() !=0)
		goto out;
	/**create table exporter*/
	if(createExporterTable() !=0)
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
IpfixDbWriter::~IpfixDbWriter() {
	mysql_close(conn);
	free(table->statement);
	free(table);
}

/**
 * Starts or resumes database
 * @param ipfixDbWriter handle obtained by calling @c createipfixDbWriter()
 */
int IpfixDbWriter::start() {
	/* unimplemented, we can't be paused - TODO: or should we? */
	return 0;
}

/**
 * Temporarily pauses database
 * @param ipfixDbWriter handle obtained by calling @c createipfixDbWriter()
 */
int IpfixDbWriter::stop() {
	/* unimplemented, we can't be paused - TODO: or should we? */
	return 0;
}


#endif
