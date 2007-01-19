#include <string.h>
#include <stdlib.h>
#include "IpfixDbReader.h"
#include "msg.h"


columnDB tabs[] = {		 
	{"dstIP", IPFIX_TYPEID_destinationIPv4Address,4},
	{"srcIP", IPFIX_TYPEID_sourceIPv4Address, 4},	
	{"srcPort", IPFIX_TYPEID_sourceTransportPort, 2},	
	{"dstPort", IPFIX_TYPEID_destinationTransportPort, 2},
	{"proto",IPFIX_TYPEID_protocolIdentifier , 1},
	{"dstTos", IPFIX_TYPEID_classOfServiceIPv4, 1},
	{"bytes", IPFIX_TYPEID_octetDeltaCount,  8},
	{"pkts", IPFIX_TYPEID_packetDeltaCount, 8},
	{"firstSwitched", IPFIX_TYPEID_flowStartSeconds, 4},
	{"lastSwitched", IPFIX_TYPEID_flowEndSeconds, 4},
	{"END"}
} ;


/***** Internal Functions ****************************************************/

void copyUintNetByteOrder(FieldData* dest, char* src, FieldType type);
int getTables(IpfixDbReader* ipfixDbReader);

columnDB* getColumnByName(const char* name);
int getColumns(IpfixDbReader* ipfixDbReader, int n);

void* readFromDB(void* ipfixDbReader_);

int dbReaderSendNewTemplate(IpfixDbReader* ipfixDbReader,DataTemplateInfo* dataTemplateInfo, int table_index);
int dbReaderSendTable(IpfixDbReader* ipfixDbReader, DataTemplateInfo* dataTemplateInfo, int n);
int dbReaderDestroyTemplate(IpfixDbReader* ipfixDbReader, DataTemplateInfo* dataTemplateInfo);

int connectToDb(IpfixDbReader* ipfixDbReader,
		const char* hostName, const char* dbName, 
		const char* username, const char* password,
		unsigned int port, uint16_t observationDomainId);


/**
 * First send a a new template, then send the dataTemplates for all stored
 * tables.
 */
void* readFromDB(void* ipfixDbReader_)
{
	int i;
	IpfixDbReader* ipfixDbReader = (IpfixDbReader*)ipfixDbReader_;

	DataTemplateInfo* dataTemplateInfo = (DataTemplateInfo*)malloc(sizeof(DataTemplateInfo));
	DbData* dbData = ipfixDbReader->dbReader->dbData;

	// TODO: make IpfixDbReader exit if exit was requested!
	pthread_mutex_lock(&ipfixDbReader->mutex);
	msg(MSG_DIALOG, "Start sending tables");
	for(i = 0; i < dbData->tableCount && i < MAX_TABLES; i++) {
		if(dbReaderSendNewTemplate(ipfixDbReader, dataTemplateInfo, i) != 0)
		{
		    msg(MSG_ERROR, "IpfixDbReader: Template error, skip table");
		    continue;
		}
		dbReaderSendTable(ipfixDbReader, dataTemplateInfo, i);
		dbReaderDestroyTemplate(ipfixDbReader, dataTemplateInfo);
		//here we can make a pause if required
		pthread_mutex_unlock(&ipfixDbReader->mutex);
		pthread_mutex_lock(&ipfixDbReader->mutex);
	}

	free(dataTemplateInfo);
	msg(MSG_DIALOG,"Sending from database is done");
	return 0;
}
/**
 * Constructs a template from the table data and sends it to all connected
 * modules.
 */
int dbReaderSendNewTemplate(IpfixDbReader* ipfixDbReader,DataTemplateInfo* dataTemplateInfo, int table_index)
{
	int i,n;
	int fieldLength  = 0;

	DbReader* dbReader = ipfixDbReader->dbReader;
	DbData* dbData = dbReader->dbData;
	
	dataTemplateInfo->id =0;
	dataTemplateInfo->preceding= 0;	
	dataTemplateInfo->fieldCount = 0;
	dataTemplateInfo->fieldInfo = NULL;
	dataTemplateInfo->dataCount = 0;
	dataTemplateInfo->dataInfo = NULL;
	dataTemplateInfo->data = NULL;
	dataTemplateInfo->userData = NULL;
		
	/**get columnsname of the table*/
	if(getColumns(ipfixDbReader, table_index) != 0) {
		msg(MSG_ERROR,"IpfixDbReader: Could not get columns for template");
		return 1;
	}

	for(i = 0; i < dbData->colCount; i++) {			
		dataTemplateInfo->fieldCount++;
		dataTemplateInfo->fieldInfo = realloc(dataTemplateInfo->fieldInfo,
						      sizeof(FieldInfo)*dataTemplateInfo->fieldCount);
		FieldInfo* fi = &dataTemplateInfo->fieldInfo[dataTemplateInfo->fieldCount - 1];	
		fi->type.id = dbData->columns[i]->ipfixId;
		fi->type.length = dbData->columns[i]->length;
		fi->type.eid = 0;
		fi->offset = fieldLength;
		fieldLength = fieldLength + fi->type.length; 
	}

	for (n = 0; n != dbReader->callbackCount; n++) {
		CallbackInfo* ci = &dbReader->callbackInfo[n];
		if (ci->dataTemplateCallbackFunction) {
			ci->dataTemplateCallbackFunction(ci->handle, &ipfixDbReader->srcId,
							 dataTemplateInfo);
			msg(MSG_DEBUG,"IpfixDbReader sent template for table %s", dbData->tableNames[table_index]);
		}
	}
	return 0;
}


void copyUintNetByteOrder(FieldData* dest, char* src, FieldType type) {
        switch (type.length) {
        case 1:
		*(uint8_t*)dest = *(uint8_t*)src;
                return;
        case 2:
		*(uint16_t*)dest = htons(*(uint16_t*)src);
                return;
        case 4:
		*(uint32_t*)dest = htonl(*(uint32_t*)src);
                return;
        case 8:
		*(uint64_t*)dest = htonll(*(uint64_t*)src);
                return;
        default:
                msg(MSG_ERROR, "IpfixDbReader: Uint with length %d unparseable", type.length);
                return;
        }
}


/**
 * Select a given table and get the values by reading
 * the database. The Typs of the values from database are
 * strings, therefore they must change into IPFIX format 
*/

int dbReaderSendTable(IpfixDbReader* ipfixDbReader, DataTemplateInfo* dataTemplateInfo, int table_index)
{
       	MYSQL_RES* dbResult = NULL;
	MYSQL_ROW dbRow = NULL;
	DbReader* dbReader = ipfixDbReader->dbReader;
	DbData* dbData = dbReader->dbData;
	int i;
	FieldData* data = (FieldData*)malloc(MAX_MSG_LEN);
	int dataLength = 0;
	unsigned delta = 0;
	unsigned flowTime = 0;
	unsigned lastFlowTime = 0;
	long long tmp;
	

	char select[STARTLEN] = "SELECT * FROM ";
	strncat(select, ipfixDbReader->dbReader->dbData->tableNames[table_index],TABLE_WIDTH);
	strcat(select," ORDER BY lastSwitched");
	/** get all data from database*/
	if(mysql_query(ipfixDbReader->conn, select) != 0) {
		msg(MSG_ERROR,"IpfixDbReader: Select on table failed. Error: %s",
		    mysql_error(ipfixDbReader->conn));
		free(data);
		return 1;
	}

	dbResult = mysql_store_result(ipfixDbReader->conn);
	msg(MSG_DEBUG,"IpfixDbReader starts sending records from table %s", dbData->tableNames[table_index]);
	while((dbRow = mysql_fetch_row(dbResult))) {
		if (delta == 0) {
			for (i = 0; i != dbData->colCount; ++i) {
				if (IPFIX_TYPEID_flowEndSeconds) {
					delta = time(NULL) - atoll(dbRow[i]);
					flowTime = lastFlowTime = atoll(dbRow[i]) + delta;
				}
			}
			if (delta == 0) {
				msg(MSG_FATAL, "IpfixDbReader: flowEndTime in first data base record missing!");
				mysql_free_result(dbResult);
				free(data);
				return 1;
			}
		}
		for(i = 0; i != dbData->colCount; ++i) {
			switch(dbData->columns[i]->ipfixId) {
			case IPFIX_TYPEID_flowEndSeconds:
			        flowTime = atoll(dbRow[i]) + delta;
			case IPFIX_TYPEID_flowStartSeconds:
				tmp = atoll(dbRow[i]) + delta;
				copyUintNetByteOrder(data + dataTemplateInfo->fieldInfo[i].offset,
						     (char*)&tmp,
						     dataTemplateInfo->fieldInfo[i].type);
				dataLength += dataTemplateInfo->fieldInfo[i].type.length;
				break;
			case IPFIX_TYPEID_octetDeltaCount:
			case IPFIX_TYPEID_packetDeltaCount:
			case IPFIX_TYPEID_destinationIPv4Address:
			case IPFIX_TYPEID_sourceIPv4Address:
			case IPFIX_TYPEID_sourceTransportPort:
			case IPFIX_TYPEID_destinationTransportPort:
			case IPFIX_TYPEID_protocolIdentifier:
			case IPFIX_TYPEID_classOfServiceIPv4:
				tmp = atoll(dbRow[i]);
				copyUintNetByteOrder(data + dataTemplateInfo->fieldInfo[i].offset,
						     (char*)&tmp,
						     dataTemplateInfo->fieldInfo[i].type);
				dataLength += dataTemplateInfo->fieldInfo[i].type.length;
				break;
			}
		}
	
		/** according to flowstarttime wait for sending the record*/
		if(flowTime != lastFlowTime) {
			time_t t = time(NULL);
			if (t > flowTime) {
				msg(MSG_ERROR, "IpfixDbReader: Sending flows too slowly");
			} else {
				sleep (flowTime - t);
			}
			lastFlowTime = flowTime;
		}
		for (i = 0; i != dbReader->callbackCount; ++i) {
			CallbackInfo* ci = &dbReader->callbackInfo[i];
			if (ci->dataDataRecordCallbackFunction) {
				ci->dataDataRecordCallbackFunction(ci->handle,
								   &ipfixDbReader->srcId,
								   dataTemplateInfo,
								   dataLength,
								   data);
				msg(MSG_INFO,"IpfixDbReader sent record");
			}
		}

	}
	mysql_free_result(dbResult);
	free(data);
	
	msg(MSG_DEBUG,"Sending from table %s done", dbData->tableNames[table_index]);

	return 0;
}


/**
 * get all tableNames in database that matches with the wildcard "h%"
 **/
int dbReaderDestroyTemplate(IpfixDbReader* ipfixDbReader, DataTemplateInfo* dataTemplateInfo)
{
	int n;
	DbReader* dbReader = ipfixDbReader->dbReader;

	for (n = 0; n != dbReader->callbackCount; n++) {
		CallbackInfo* ci = &dbReader->callbackInfo[n];
		if (ci->dataTemplateDestructionCallbackFunction) {
			ci->dataTemplateDestructionCallbackFunction(ci->handle, &ipfixDbReader->srcId,
							 dataTemplateInfo);
			msg(MSG_DEBUG,"IpfixDbReader destroyed template");
		}
	}

	free(dataTemplateInfo->fieldInfo);
	return 0;
}




/**
 * get all tableNames in database that matches with the wildcard "h%"
 **/
int getTables(IpfixDbReader* ipfixDbReader)
{
	DbData* dbData = ipfixDbReader->dbReader->dbData;
	int i = 0;
	char* wild = "h%";
	MYSQL_RES* dbResult = NULL;
	MYSQL_ROW dbRow = NULL;
	
	dbResult = mysql_list_tables(ipfixDbReader->conn, wild);
	if(dbResult == 0) {
		msg(MSG_FATAL,"There are no flow tables in database %s", ipfixDbReader->dbName);	
		return 1;
	}
	
	if(mysql_num_rows(dbResult) > MAX_TABLES) {
		msg(MSG_ERROR,"There are too many flow tables in the database. Only the first MAX_TABLES=%i tables can be read.", MAX_TABLES);
	}

	while(( dbRow = mysql_fetch_row(dbResult)) && i < MAX_TABLES) {
		char *table = (char*)malloc(sizeof(char) * TABLE_WIDTH);
		strcpy(table,dbRow[0]);
		dbData->tableNames[i] = table;
		dbData->tableCount++;
		i++;
	}

	mysql_free_result(dbResult);
	
	return 0;
}

columnDB* getColumnByName(const char* name)
{
	int i;
	for (i = 0; strcmp(tabs[i].cname, "END"); ++i) {
		if (!strcmp(tabs[i].cname, name)) {
			return &tabs[i];
		}
	}
	return NULL;
}

/**
 * Get the names of columns
 */
int getColumns(IpfixDbReader* ipfixDbReader, int table_index)
{
	DbData* dbData = ipfixDbReader->dbReader->dbData;
	MYSQL_RES* dbResult = NULL;
	MYSQL_ROW dbRow = NULL;
	
	char showcolStr[STARTLEN] = "SHOW COLUMNS FROM ";
	/* get column names from table table_index */
	strncat(showcolStr, dbData->tableNames[table_index],strlen(dbData->tableNames[table_index])+1);
	if(mysql_query(ipfixDbReader->conn, showcolStr) != 0) {	
		msg(MSG_ERROR,"Show columns on table %s failed. Error: %s",
		    mysql_error(ipfixDbReader->conn));
		return 1;
	}
	
	dbResult = mysql_store_result(ipfixDbReader->conn);
	
	if(dbResult == 0) {
		msg(MSG_FATAL,"There are no Columns in the table");	
		return 1;
	}
	
	// TODO: don't we have to free the result of mysql_fetch_row?????
	dbData->colCount = 0;
	while((dbRow = mysql_fetch_row(dbResult))) {
		if(strcmp(dbRow[0],"exporterID") != 0) {
			if(dbData->colCount > MAX_COL) {
				msg(MSG_ERROR,"Too many columns in table");
				mysql_free_result(dbResult);
				return 1;
			}

			dbData->columns[dbData->colCount] = getColumnByName(dbRow[0]);
			msg(MSG_DEBUG, "Column name: %s", dbData->columns[dbData->colCount]->cname);
			dbData->colCount++;
		}
	}

	mysql_free_result(dbResult);

	return 0;
}


int connectToDb(IpfixDbReader* ipfixDbReader,
		const char* hostName, const char* dbName, 
		const char* userName, const char* password,
		unsigned int port, uint16_t observationDomainId)

{
	/** get the mysl init handle*/
	ipfixDbReader->conn = mysql_init(0); 
	if(ipfixDbReader->conn == 0) {
		msg(MSG_FATAL,"Get MySQL connect handle failed. Error: %s",
		    mysql_error(ipfixDbReader->conn));
		return 1;
	} else {
		msg(MSG_DEBUG,"mysql init successfull");
	}
	/**Initialize structure members IpfixDbWriter*/
	ipfixDbReader->hostName = hostName;
	ipfixDbReader->dbName = dbName;
	ipfixDbReader->userName = userName;
	ipfixDbReader->password = password;
	ipfixDbReader->portNum = port;
	ipfixDbReader->socketName = 0;	  		
	ipfixDbReader->flags = 0;
	ipfixDbReader->srcId.observationDomainId = observationDomainId;
	/**Initialize structure members DbReader*/
	ipfixDbReader->dbReader->callbackInfo = NULL;
	ipfixDbReader->dbReader->callbackCount = 0;
	/**Initialize structure members DbData*/
	ipfixDbReader->dbReader->dbData->colCount = 0;
	ipfixDbReader->dbReader->dbData->tableCount = 0;
	
	/**Connect to Database*/
	if (!mysql_real_connect(ipfixDbReader->conn,
				ipfixDbReader->hostName,
				ipfixDbReader->userName,ipfixDbReader->password,
				0, ipfixDbReader->portNum, ipfixDbReader->socketName,
				ipfixDbReader->flags)) {
		msg(MSG_FATAL,"Connection to database failed. Error: %s",
		    mysql_error(ipfixDbReader->conn));
		return 1;
	}

	return 0;
}

/***** Exported Functions ****************************************************/


/**
 * Initializes internal structures.
 * To be called on application startup
 * @return 0 on success
 */
int initializeIpfixDbReaders() {
	return 0;
}


/**
 * Deinitializes internal structures.
 * To be called on application shutdown
 * @return 0 on success
 */
int deinitializeIpfixDbReaders() {
	return 0;
}

/**
 * Starts or resumes database
 * @param ipfixDbReader handle obtained by calling @c createipfixDbReader()
 */
int startIpfixDbReader(IpfixDbReader* ipfixDbReader) {
	pthread_mutex_unlock(&ipfixDbReader->mutex);
	return 0;
}

/**
 * Temporarily pauses database
 * @param ipfixDbReader handle obtained by calling @c createipfixDbReader()
 */
int stopIpfixDbReader(IpfixDbReader* ipfixDbReader) {
	pthread_mutex_lock(&ipfixDbReader->mutex);
	return 0;
}

/**
 * Frees memory used by an ipfixDbReader
 * @param ipfixDbWriter handle obtained by calling @c createipfixDbReader()
 */
int destroyIpfixDbReader(IpfixDbReader* ipfixDbReader) {
	mysql_close(ipfixDbReader->conn);
	if (!pthread_mutex_destroy(&ipfixDbReader->mutex)) {
		msg(MSG_ERROR, "Could not destroy mutex");
	}
	free(ipfixDbReader->dbReader->dbData);
	free(ipfixDbReader->dbReader);
	free(ipfixDbReader);

	return 0;
}

/**
 * Creates a new ipfixDbReader. Do not forget to call @c startipfixDbReader() to begin reading from Database
 * @return handle to use when calling @c destroyipfixDbRreader()
 */
IpfixDbReader* createIpfixDbReader(const char* hostName, const char* dbName, 
				   const char* userName, const char* password,
				   unsigned int port, uint16_t observationDomainId)
{
	IpfixDbReader* ipfixDbReader = (IpfixDbReader*)malloc(sizeof(IpfixDbReader));
	if (!ipfixDbReader) {
		msg(MSG_ERROR, "Could not allocate IpfixDbReader");
		goto out0;
	}

	if (pthread_mutex_init(&ipfixDbReader->mutex, NULL)) {
		msg(MSG_FATAL, "Could not init mutex");
		goto out1;
	}

        if (pthread_mutex_lock(&ipfixDbReader->mutex)) {
                msg(MSG_FATAL, "Could not lock mutex");
                goto out1;
        }

	DbReader* dbReader = (DbReader*)malloc(sizeof(DbReader));
	if (!ipfixDbReader) {
		msg(MSG_ERROR, "Could not allocate DbReader");
		goto out1;
	}

	DbData* dbData = (DbData*)malloc(sizeof(DbData));
	if (!dbData) {
		msg(MSG_ERROR, "Could not allocate dbData");
		goto out2;
	}
	
	ipfixDbReader->dbReader = dbReader;
	dbReader->dbData = dbData;
	if (connectToDb(ipfixDbReader, hostName, dbName, userName,
			password, port, observationDomainId)) {
		goto out3;
	}
	msg(MSG_DEBUG,"Connected to database");
	
	/** use database  with db_name**/	
	if(mysql_select_db(ipfixDbReader->conn, ipfixDbReader->dbName) !=0) {
		msg(MSG_FATAL,"Database %s not selectable", ipfixDbReader->dbName);	
		goto out3;
	} else {
		msg(MSG_DEBUG,"Database %s selected", ipfixDbReader->dbName);
	}
	/** get tableNames of the database*/
	if(getTables(ipfixDbReader) != 0) {
		msg(MSG_ERROR,"Error in function getTables");
		goto out3;
	}
	/**initialize columns**/
	dbData->colCount = 0;

	if (pthread_create(&ipfixDbReader->thread, 0, readFromDB, ipfixDbReader)) {
		msg(MSG_FATAL, "Could not create dbRead thread");
                goto out3;
	}
	
	return ipfixDbReader;

out3:
	free(dbData);
out2:
	free(dbReader);
out1:
	free(ipfixDbReader);
out0:	
	return NULL;
}


/**
 * Adds a set of callback functions to the list of functions to call
 * when Message from database is read.
 * @param ipfixDbReader IpfixDbReader to set the callback function for
 * @param handles set of callback functions
 */
void addIpfixDbReaderCallbacks(IpfixDbReader* ipfixDbReader, CallbackInfo handles) 
{
	DbReader* dbReader = ipfixDbReader->dbReader;
	int n = ++dbReader->callbackCount;
	dbReader->callbackInfo = (CallbackInfo*)realloc(dbReader->callbackInfo, 
							n * sizeof(CallbackInfo));
	memcpy(&dbReader->callbackInfo[n-1], &handles, sizeof(CallbackInfo));
}
