#ifdef DB_SUPPORT_ENABLED

#include <string.h>
#include <stdlib.h>
#include "IpfixDbWriter.h"
#include "msg.h"


/***** Internal types ******************************************************/

/**
 * Identify the depency between columns names and 
 * IPFIX_TYPEID working with a char pointer array
 * in this array there is also standing  the defaultvalue
 * of the IPFIX_TYPEID and the datatype to store in database
*/
struct Column{
        char* cname;       /** column name */
        int ipfixId;       /** IPFIX_TYPEID */
        char* dataType;    /** which datatype to store in database */
        int defaultValue;  /** when no IPFIX_TYPEID is stored in the record,
                            *  use defaultvalue to store in database
                            */
};


/***** Global Variables ******************************************************/
/**
* 	is needed to determine "now" time and the time of flowstartsseconds
*/
struct tm* timeNow;

/**
*	column names as a array of char pointer
*/
char *columnsNames[] = {"srcIP", "dstIP", "srcPort", "dstPort", "proto", "dstTos","bytes","pkts","firstSwitched","lastSwitched","exporterID",0};					  
 
 /**
 *	struct to identify column names with IPFIX_TYPEID an the dataType to store in database
 *	ExporterID is no IPFIX_TYPEID, its user specified
 */
struct Column identify [] = {
	{"dstIP", IPFIX_TYPEID_destinationIPv4Address, "INTEGER(10) UNSIGNED",0},
	{"srcIP", IPFIX_TYPEID_sourceIPv4Address, "INTEGER(10) UNSIGNED", 0},	
	{"srcPort", IPFIX_TYPEID_sourceTransportPort,  "SMALLINT(5) UNSIGNED", 0},	
	{"dstPort", IPFIX_TYPEID_destinationTransportPort,  "SMALLINT(5) UNSIGNED",0},
	{"proto",IPFIX_TYPEID_protocolIdentifier , "TINYINT(3) UNSIGNED", 0},
	{"dstTos", IPFIX_TYPEID_classOfServiceIPv4, "TINYINT(3) UNSIGNED", 0},
	{"bytes", IPFIX_TYPEID_octetDeltaCount,  "BIGINT(20) UNSIGNED", 0},
	{"pkts", IPFIX_TYPEID_packetDeltaCount, "BIGINT(20) UNSIGNED", 0},
	{"firstSwitched", IPFIX_TYPEID_flowStartSeconds,  "INTEGER(10) UNSIGNED", 0},
	{"lastSwitched", IPFIX_TYPEID_flowEndSeconds,  "INTEGER(10) UNSIGNED", 0},
	//{"firstSwitched", 22,  "INTEGER(10) UNSIGNED", 0},
	//{"lastSwitched", 21,  "INTEGER(10) UNSIGNED", 0},
	{"exporterID",EXPORTERID, "SMALLINT(5) UNSIGNED", 0},
	{"END"}
} ;
	
/***** Internal Functions ****************************************************/

int createDB(IpfixDbWriter* ipfixDbWriter);
int  createDBTable(IpfixDbWriter* ipfixDbWriter,Table* table, char* TableName);
int createExporterTable(IpfixDbWriter* ipfixDbWriter);

int writeDataRecord(void* ipfixDbWriter,SourceID* sourceID, TemplateInfo* templateInfo, uint16_t length, FieldData* data);
int writeDataDataRecord(void* ipfixDbWriter,SourceID* sourceID, DataTemplateInfo* dataTemplateInfo, uint16_t length, FieldData* data);
char* getRecData(IpfixDbWriter* ipfixDbWriter,Table* table,SourceID* sourceID,DataTemplateInfo* dataTemplateInfo,uint16_t length,FieldData* data);

int getExporterID(IpfixDbWriter* ipfixDbWriter,Table* table, SourceID* sourceID);

char* getTableName(IpfixDbWriter* ipfixDbWriter,Table* table, uint64_t flowstartsec);
char* getTableNamDependTime(char* tablename,uint64_t flowstartsec);
uint64_t getTableStartTime(uint64_t flowstartsec);
uint64_t getTableEndTime(uint64_t StartTime);

int writeToDb(IpfixDbWriter* ipfixDbWriter, Table* table, Statement* statement);

uint64_t getdata(FieldType type, FieldData* data);
uint64_t getIPFIXValue(FieldType type, FieldData* data);
uint32_t getdefaultIPFIXdata(int ipfixtype);

uint32_t getipv4address(FieldType type, FieldData* data);


/**
* create the database given by the name dbnam->dbn
*/
int createDB(IpfixDbWriter* ipfixDbWriter)
{
	/**Is there a already a database with the same name - drop it*/
        /* gerhard: let's keep the database, we do not want to lose data
	char dropDb[STARTLEN];
	strcpy(dropDb, "DROP DATABASE IF EXISTS ");
	strncat(dropDb, ipfixDbWriter->dbName,strlen(ipfixDbWriter->dbName)+1);
	msg(MSG_ERROR, "%s", dropDb);
	if(mysql_query(ipfixDbWriter->conn, dropDb) != 0 ) {
		msg(MSG_FATAL,"Drop of exists database failed. Error: %s",
		    mysql_error(ipfixDbWriter->conn));
		return 1;
	} */

	/** make query string to create database**/
	char  createDbStr[STARTLEN] ;
	strcpy(createDbStr,"CREATE DATABASE IF NOT EXISTS ");
	strncat(createDbStr,ipfixDbWriter->dbName,strlen(ipfixDbWriter->dbName)+1);
	/**create database*/
	if(mysql_query(ipfixDbWriter->conn, createDbStr) != 0 ) {
		msg(MSG_FATAL,"IpfixDbWriter: Creation of database %s failed. Error: %s",
		    ipfixDbWriter->dbName, mysql_error(ipfixDbWriter->conn));
		return 1;
	} else {
		msg(MSG_DEBUG,"Database %s created",ipfixDbWriter->dbName);
	}
	/** use database  with dbName**/	
	if(mysql_select_db(ipfixDbWriter->conn, ipfixDbWriter->dbName) !=0) {
		msg(MSG_FATAL,"IpfixDbWriter: Database %s not selectable. Error: %s",
		    ipfixDbWriter->dbName, mysql_error(ipfixDbWriter->conn));
		return 1;
	} else {
		msg(MSG_DEBUG,"Database %s selected", ipfixDbWriter->dbName);
	}
	return 0;
}

int createExporterTable(IpfixDbWriter* ipfixDbWriter)
{
	/**is there already a table with the same name - drop it */
        /* gerhard: let's keep the database, we do not want to lose data
	char dropExporterTab[STARTLEN];
	strcpy(dropExporterTab,"DROP TABLE IF EXISTS exporter");
	if(mysql_query(ipfixDbWriter->conn, dropExporterTab) != 0) {
		msg(MSG_FATAL,"Drop of exists exporter table failed. Error: %s",
		    mysql_error(ipfixDbWriter->conn));
		return 1;
	}
	*/
	/** create table exporter*/

	// TODO: make this less ugly
	char createExporterTab[STARTLEN+(3 * COL_WIDTH)];
	strcpy(createExporterTab,"CREATE TABLE IF NOT EXISTS exporter (id SMALLINT(5) NOT NULL AUTO_INCREMENT,sourceID INTEGER(10) UNSIGNED DEFAULT NULL,srcIP INTEGER(10) UNSIGNED DEFAULT NULL,PRIMARY KEY(id))");
	if(mysql_query(ipfixDbWriter->conn,createExporterTab) != 0) {
		msg(MSG_FATAL,"IpfixDbWriter: Creation of table Exporter failed. Error: %s",
		    mysql_error(ipfixDbWriter->conn));
		return 1;
	} else {
		msg(MSG_DEBUG,"Exporter table created");
	}
	return 0;
}

/**
* 	Create the table of the database
*/
int createDBTable(IpfixDbWriter* ipfixDbWriter,Table* table, char* tablename)
{
	int i, j ;
	char createTableStr[STARTLEN+(table->countCol* COL_WIDTH)];
	strcpy(createTableStr , "CREATE TABLE IF NOT EXISTS ");
	strncat(createTableStr,tablename,strlen(tablename)+1);
	strncat(createTableStr," (",(2*sizeof(char))+1);
	/**collect the names for columns and the dataTypes for the table in a string*/
	for(i=0; i < table->countCol; i++) {
		for(j=0; strcmp(identify[j].cname,"END") != 0 ;j++) {
			/**if columnsNames equal identify.cname then ...*/
			if( strcmp(columnsNames[i], identify[j].cname) == 0) {
				strncat(createTableStr,identify[j].cname,strlen(identify[j].cname)+1);
				strncat(createTableStr," ",sizeof(char)+1);
				strncat(createTableStr,identify[j].dataType,strlen(identify[j].dataType)+1);
				if( i  != table->countCol-1) {
					strncat(createTableStr,",",sizeof(char)+1);
				}
			}
		}
	}
	strncat(createTableStr,")",sizeof(char)+1);
	
	/**Is there a already a table with the same name - drop it*/
	/* gerhard: oh no, do not drop any flow tables during operation
	char dropTable[STARTLEN];
	strcpy(dropTable,"DROP TABLE IF EXISTS ");
	strncat(dropTable, tablename,strlen(tablename)+1);
	if(mysql_query(ipfixDbWriter->conn, dropTable) != 0) {
		msg(MSG_FATAL,"Drop of exists %s table failed. Error: %s",tablename,
		    mysql_error(ipfixDbWriter->conn));
		return 1;
	}
	*/
	/** create table*/
	if(mysql_query(ipfixDbWriter->conn,createTableStr) != 0) {
		msg(MSG_FATAL,"IpfixDbWriter: Creation of table failed. Error: %s",
		    mysql_error(ipfixDbWriter->conn));
		return 1;
	} else {
		msg(MSG_DEBUG, "Table %s created ",tablename);
	}
	return 0;
}

/**
*	function receive the DataRecord or DataDataRecord when callback is started
*/
int  writeDataDataRecord(void* ipfixDbWriter_, SourceID* sourceID, DataTemplateInfo* dataTemplateInfo, uint16_t length, FieldData* data)
{
        IpfixDbWriter *ipfixDbWriter = (IpfixDbWriter*)ipfixDbWriter_;
	Table *tabl = ipfixDbWriter->table;
	Statement* statemen = tabl->statement;

	DPRINTF("Processing data record\n");

	/** if the writeToDb process not ready - drop record*/
	if(statemen->statemBuffer[statemen->maxStatements-1] != NULL) {
		msg(MSG_ERROR,"IpfixDbWriter: Statement buffer is full, writing to DB in progress? - drop record");
		return 1;
	}
	
	/** sourceid null ? use default*/
	/* overwrite sourceid if defined */
	if(ipfixDbWriter->srcId.observationDomainId != 0 || sourceID == NULL) {
		sourceID = &ipfixDbWriter->srcId;
	}

	/** make a sql insert statement from the recors data */
	char* insertTableStr = getRecData(ipfixDbWriter, tabl, sourceID,
					  dataTemplateInfo, length, data);
	DPRINTF("Insert statement: %s\n",insertTableStr);	
	
	/** if statement counter lower as  max count of statement then insert record in statemenBuffer*/
	if(statemen->statemReceived < statemen->maxStatements) {	
		statemen->statemBuffer[statemen->statemReceived] = insertTableStr;
		/** statemBuffer is filled ->  insert in table*/	
		if(statemen->statemReceived == statemen->maxStatements-1) {
                        msg(MSG_INFO, "Writing buffered records to database");
			writeToDb(ipfixDbWriter, tabl, statemen);
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
int writeDataRecord(void* ipfixDbWriter, SourceID* sourceID, TemplateInfo* templateInfo, uint16_t length, FieldData* data)
{
	DataTemplateInfo dataTemplateInfo;
	
	dataTemplateInfo.id = 0;
	dataTemplateInfo.preceding = 0;
	dataTemplateInfo.fieldCount = templateInfo->fieldCount;  /**< number of regular fields */
	dataTemplateInfo.fieldInfo = templateInfo->fieldInfo;   /**< array of FieldInfos describing each of these fields */
	dataTemplateInfo.dataCount = 0;   /**< number of fixed-value fields */
	dataTemplateInfo.dataInfo = NULL;    /**< array of FieldInfos describing each of these fields */
	dataTemplateInfo.data = NULL;        /**< data start pointer for fixed-value fields */
	dataTemplateInfo.userData = templateInfo->userData;    /**< pointer to a field that can be used by higher-level modules */
	
	DPRINTF("receiveRec calls receiveDataRec\n");	

	return writeDataDataRecord(ipfixDbWriter, sourceID, &dataTemplateInfo, length, data);
}

/**
*	loop over the DataTemplateInfo (fieldinfo,datainfo) to get the IPFIX values to store in database
*/
char* getRecData(IpfixDbWriter* ipfixDbWriter,Table* table, SourceID* sourceID,
		 DataTemplateInfo* dataTemplateInfo,uint16_t length, FieldData* data)
{
	int i ,j, k, n;
	uint64_t intdata = 0;
	uint32_t flowstartsec = 0;
	/**begin query string for insert statement*/
	char insert[STARTLEN+(table->countCol * INS_WIDTH)];
	strcpy(insert,"INSERT INTO ");
	
	/**make string for the column names*/
	char ColNames[table->countCol * INS_WIDTH];
	strcpy(ColNames," (");
	
	/**make string for the values  given by the IPFIX_TYPEID stored in the record*/ 
	char ColValues[table->countCol * INS_WIDTH]; 
	strcpy(ColValues," VALUES (");	
	
	char stringtmp[INS_WIDTH];// needed to cast from char to string
			
	/**loop over the columname and loop over the IPFIX_TYPEID of the record
	   to get the corresponding */
	/**data to store and make insert statement*/
	for( i=0; i < table->countCol; i++) {
		for( j=0; strcmp(identify[j].cname,"END") != 0; j++) { 
			if(columnsNames[i] == identify[j].cname) {	
				int notfound = 1;
				strncat(ColValues,"'",sizeof(char)+1);
				strncat(ColNames,columnsNames[i],strlen(columnsNames[i])+1);
				if( i != table->countCol-1)
					strncat(ColNames,",",sizeof(char)+1);
				if( i == table->countCol-1)
					strncat(ColNames,") ",(2*sizeof(char))+1);
				if(dataTemplateInfo->fieldCount > 0) {
					for(k=0; k < dataTemplateInfo->fieldCount; k++) {	
						if(dataTemplateInfo->fieldInfo[k].type.id == identify[j].ipfixId) {
							notfound = 0;						
							intdata = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset));
							sprintf(stringtmp,"%Lu",(uint64_t)intdata);
							strncat(ColValues,stringtmp,strlen(stringtmp)+1);
							strncat(ColValues,"'",sizeof(char)+1);
							if(identify[j].ipfixId == IPFIX_TYPEID_flowStartSeconds) {
								flowstartsec = intdata;
							}
							if( i !=  table->countCol-1)
								strncat(ColValues,",",sizeof(char)+1);
							if( i == table->countCol-1)
								strncat(ColValues,")",sizeof(char)+1);	
							break;
						}
					}	
				}
				if( dataTemplateInfo->dataCount > 0 && notfound) {
					for(n=0; n < dataTemplateInfo->dataCount; n++) {
						if(dataTemplateInfo->dataInfo[n].type.id == identify[j].ipfixId) {
							notfound = 0;
							intdata = getdata(dataTemplateInfo->dataInfo[n].type,(dataTemplateInfo->data+dataTemplateInfo->dataInfo[n].offset));
							sprintf(stringtmp,"%Lu",(uint64_t)intdata);
							strncat(ColValues,stringtmp,strlen(stringtmp)+1);
							strncat(ColValues,"'",sizeof(char)+1);
							if( i != table->countCol-1)
								strncat(ColValues,",",sizeof(char)+1);
							if( i == table->countCol-1)
								strncat(ColValues,")",sizeof(char)+1);					
							break;
						}
					}
				}
				if(notfound) {	
					if(identify[j].ipfixId == EXPORTERID) {
						/**lookup exporter buffer to get exporterID from sourcID and expIp**/
						uint32_t expID = getExporterID(ipfixDbWriter, table, sourceID);
						sprintf(stringtmp,"%u",(uint32_t)expID);
						strncat(ColValues,stringtmp,strlen(stringtmp)+1);
						strncat(ColValues,"'",sizeof(char)+1);
					} else {
						intdata = getdefaultIPFIXdata(identify[j].ipfixId);
						sprintf(stringtmp,"%Lu",(uint64_t)intdata);
						strncat(ColValues,stringtmp,strlen(stringtmp)+1);
						strncat(ColValues,"'",sizeof(char)+1);
					}
					if( i != table->countCol-1)
						strncat(ColValues,",",sizeof(char)+1);
					if( i == table->countCol-1)
						strncat(ColValues,")",sizeof(char)+1);					
				}
			}
		}
	}
	
	/**make hole query string for the insert statement*/
	char tablename[TABLE_WIDTH] ;
	char* tablen = getTableName(ipfixDbWriter, table, flowstartsec);
	strcpy(tablename, tablen);
	/** Insert statement = INSERT INTO + tablename +  Columnsname + Values of record*/
	strncat(insert, tablename,strlen(tablename)+1);  
	strncat(insert,ColNames,strlen(ColNames)+1);
	strncat(insert,ColValues, strlen(ColValues)+1);
	
	
	char* insertTableStr = (char*) malloc((strlen(insert)+1)*sizeof(char));
	strcpy(insertTableStr,insert);
		
	return insertTableStr;
}


/**
*	Function writes the content of the statemBuffer to database
*	statemBuffer consist of single insert statements
*/
int writeToDb(IpfixDbWriter* ipfixDbWriter,Table* table, Statement* statement)
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
	if(mysql_query(ipfixDbWriter->conn, LockTables) != 0) {
		msg(MSG_ERROR,"IpfixDbWriter: Lock of table failed. Error: %s",
		    mysql_error(ipfixDbWriter->conn));
		return 1;		    
	}
	/**Write the insert statement to database*/
	for(i=0; i != statement->maxStatements; i++) {
		if(mysql_query(ipfixDbWriter->conn, statement->statemBuffer[i]) != 0) {
			msg(MSG_ERROR,"IpfixDbWriter: Insert of records failed",
			    mysql_error(ipfixDbWriter->conn));
			return 1;
		} else {
			DPRINTF("Record inserted\n");
		}
		free(statement->statemBuffer[i]);
		statement->statemBuffer[i] = NULL;
	}
	
	char UnLockTable[STARTLEN] = "UNLOCK TABLES";
	if(mysql_query(ipfixDbWriter->conn, UnLockTable) != 0) {
		msg(MSG_ERROR,"IpfixDbWriter: Unlock of tables failed",
		    mysql_error(ipfixDbWriter->conn));
		return 1;
	}
	statement->statemReceived = 0;
	msg(MSG_INFO,"Write to database is complete");
	return 0;
}

/**
*	Returns the tablename of a record according flowstartsec
*/
char* getTableName(IpfixDbWriter* ipfixDbWriter,Table*  table , uint64_t flowstartsec)
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
	if(createDBTable(ipfixDbWriter, table, table->tableBuffer[table->countBuffTable].TableName) != 0) {	
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
int getExporterID(IpfixDbWriter* ipfixDbWriter,Table* table, SourceID* sourceID)
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
		
	if(mysql_query(ipfixDbWriter->conn, selectStr) != 0) {
		msg(MSG_ERROR,"IpfixDbWriter: Select on exporter table failed. Error: %s",
		    mysql_error(ipfixDbWriter->conn));
		return 0;// If a failure occurs, return exporterID = 0
	} 

	dbResult = mysql_store_result(ipfixDbWriter->conn);
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
	if(mysql_query(ipfixDbWriter->conn, LockExporter) != 0) {
		msg(MSG_ERROR,"IpfixDbWriter: Lock of exporter table failed. Error: %s",
		    mysql_error(ipfixDbWriter->conn));
		return 0;
	}
	
	if(mysql_query(ipfixDbWriter->conn, insertStr) != 0) {
		msg(MSG_ERROR,"IpfixDbWriter: Insert in exporter table failed. Error: %s",
		    ipfixDbWriter->conn);
		/**Unlock the table when a failure occur*/
		if(mysql_query(ipfixDbWriter->conn, UnLockExporter) != 0) {
			msg(MSG_ERROR,"IpfixDbWriter: UnLock of exporter table failed. Error: %s",
			    mysql_error(ipfixDbWriter->conn));
			return 0;
		}
		return 0;
        }

	exporterID = mysql_insert_id(ipfixDbWriter->conn); 
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
		
	if(mysql_query(ipfixDbWriter->conn, UnLockExporter) != 0) {
		msg(MSG_ERROR,"IpfixDbWriter: UnLock of exporter table failed. Error: %s",
		    mysql_error(ipfixDbWriter->conn));
		return 0;
	}
	return exporterID;
}

/**
 *	Get data of the record is given by the IPFIX_TYPEID
*/
uint64_t getdata(FieldType type, FieldData* data)
{
	if(type.id == IPFIX_TYPEID_sourceIPv4Address || type.id == IPFIX_TYPEID_destinationIPv4Address)
		return getipv4address(type, data);
	else
		return getIPFIXValue(type, data);
}
/**
 *	determine the ipv4address of the data record
 */
uint32_t getipv4address( FieldType type, FieldData* data)
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
uint64_t getIPFIXValue(FieldType type, FieldData* data)
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
	for( i=0; strcmp(identify[i].cname, "END") != 0; i++) {
		if(ipfixtype_id == identify[i].ipfixId)	{
			return identify[i].defaultValue;
		}
	}
	return 0;
}

/***** Exported Functions ****************************************************/

/**
 * Initializes internal structures.
 * To be called on application startup
 * @return 0 on success
 */
int initializeIpfixDbWriters() {
	return 0;
}
																				 					     
/**
 * Deinitializes internal structures.
 * To be called on application shutdown
 * @return 0 on success
 */
int deinitializeIpfixDbWriters() {
	return 0;
}																				 					     
												 					     
/**
 * Creates a new ipfixDbWriter. Do not forget to call @c startipfixDbWriter() to begin writing to Database
 * @return handle to use when calling @c destroyipfixDbWriter()
 */
IpfixDbWriter* createIpfixDbWriter(const char* hostName, const char* dbName,
                                   const char* userName, const char* password,
                                   unsigned int port, uint16_t observationDomainId,
                                   int maxStatements)
{	
	IpfixDbWriter* ipfixDbWriter = (IpfixDbWriter*)malloc(sizeof(IpfixDbWriter));
	Table* tabl = (Table*)malloc(sizeof(Table));
	Statement* statemen = (Statement*)malloc(sizeof(Statement));
	statemen->statemBuffer = (char**)malloc(sizeof(char**)*maxStatements);
	statemen->maxStatements = maxStatements;
	
	ipfixDbWriter->conn = mysql_init(0);  /** get the mysl init handle*/
	if(ipfixDbWriter->conn == 0) {
		msg(MSG_FATAL,"IpfixDbWriter: Get MySQL connect handle failed. Error: %s",
		    mysql_error(ipfixDbWriter->conn));
		goto out;
	} else {
		msg(MSG_DEBUG,"IpfixDbWriter got MySQL init handler");
	}
	/**Initialize structure members IpfixDbWriter*/
	ipfixDbWriter->hostName = hostName;
	ipfixDbWriter->dbName = dbName;
	ipfixDbWriter->userName = userName;    		
	ipfixDbWriter->password = password;		
	ipfixDbWriter->portNum = port;
	ipfixDbWriter->socketName = 0;
	ipfixDbWriter->flags = 0;
	ipfixDbWriter->srcId.observationDomainId = observationDomainId;
	/**Initialize structure members Table*/	  
	ipfixDbWriter->table = tabl ;
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
	for(i=0; columnsNames[i] !=0; i++)
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
	if (!mysql_real_connect(ipfixDbWriter->conn,
				ipfixDbWriter->hostName, ipfixDbWriter->userName,
				ipfixDbWriter->password, 0, ipfixDbWriter->portNum,
				ipfixDbWriter->socketName, ipfixDbWriter->flags)) {
		msg(MSG_FATAL,"IpfixDbWriter: Connection to database failed. Error: %s",
		    mysql_error(ipfixDbWriter->conn));
		goto out;
	} else {
		msg(MSG_DEBUG,"IpfixDbWriter succesfully connected to database");
	}
	/** create Database*/
	if(createDB(ipfixDbWriter) !=0)
		goto out;
	/**create table exporter*/
	if(createExporterTable(ipfixDbWriter) !=0)
		goto out;
	
	return ipfixDbWriter;
	
out: 
	destroyIpfixDbWriter(ipfixDbWriter);
		
	return NULL;	
}

/**
 * Frees memory used by an ipfixDbWriter
 * @param ipfixDbWriter handle obtained by calling @c createipfixDbWriter()
 */
int destroyIpfixDbWriter(IpfixDbWriter* ipfixDbWriter) {
	mysql_close(ipfixDbWriter->conn);
	free(ipfixDbWriter->table->statement);
	free(ipfixDbWriter->table);
	free(ipfixDbWriter);

	return 0;
}

/**
 * Starts or resumes database
 * @param ipfixDbWriter handle obtained by calling @c createipfixDbWriter()
 */
int  startIpfixDbWriter(IpfixDbWriter* ipfixDbWriter) {
	/* unimplemented, we can't be paused - TODO: or should we? */
	return 0;
}

/**
 * Temporarily pauses database
 * @param ipfixDbWriter handle obtained by calling @c createipfixDbWriter()
 */
int stopIpfixDbWriter(IpfixDbWriter* ipfixDbWriter) {
	/* unimplemented, we can't be paused - TODO: or should we? */
	return 0;
}

CallbackInfo getIpfixDbWriterCallbackInfo(IpfixDbWriter *ipfixDbWriter) {
	CallbackInfo ci;
	bzero(&ci, sizeof(CallbackInfo));
	ci.handle = ipfixDbWriter;
	ci.dataRecordCallbackFunction = writeDataRecord;
	ci.dataDataRecordCallbackFunction = writeDataDataRecord;
	
	return ci;
}

#endif
