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
#include "IpfixDbReader.hpp"
#include "common/msg.h"

/***** Internal Functions ****************************************************/

void copyUintNetByteOrder(IpfixRecord::Data* dest, char* src, IpfixRecord::FieldInfo::Type type);

/**
 * First send a a new template, then send the dataTemplates for all stored
 * tables.
 */
void* IpfixDbReader::readFromDB(void* ipfixDbReader_)
{
	IpfixDbReader* ipfixDbReader = (IpfixDbReader*)ipfixDbReader_;
	int i;

	boost::shared_ptr<IpfixRecord::DataTemplateInfo> dataTemplateInfo(new IpfixRecord::DataTemplateInfo);
	DbData* dbData = ipfixDbReader->dbReader->dbData;

	// TODO: make IpfixDbReader exit if exit was requested!
	pthread_mutex_lock(&ipfixDbReader->mutex);
	msg(MSG_DIALOG, "Start sending tables");
	for(i = 0; i < dbData->tableCount && i < MAX_TABLES; i++) {
		if(ipfixDbReader->dbReaderSendNewTemplate(dataTemplateInfo, i) != 0)
		{
		    msg(MSG_ERROR, "IpfixDbReader: Template error, skip table");
		    continue;
		}
		ipfixDbReader->dbReaderSendTable(dataTemplateInfo, i);
		ipfixDbReader->dbReaderDestroyTemplate(dataTemplateInfo);
		//here we can make a pause if required
		pthread_mutex_unlock(&ipfixDbReader->mutex);
		pthread_mutex_lock(&ipfixDbReader->mutex);
	}

	msg(MSG_DIALOG,"Sending from database is done");
	return 0;
}
/**
 * Constructs a template from the table data and sends it to all connected
 * modules.
 */
int IpfixDbReader::dbReaderSendNewTemplate(boost::shared_ptr<IpfixRecord::DataTemplateInfo> dataTemplateInfo, int table_index)
{
	int i;
	int fieldLength  = 0;

	DbData* dbData = dbReader->dbData;
	
	dataTemplateInfo->templateId =0;
	dataTemplateInfo->preceding= 0;	
	dataTemplateInfo->fieldCount = 0;
	dataTemplateInfo->fieldInfo = NULL;
	dataTemplateInfo->dataCount = 0;
	dataTemplateInfo->dataInfo = NULL;
	dataTemplateInfo->data = NULL;
	dataTemplateInfo->userData = NULL;
		
	/**get columnsname of the table*/
	if(getColumns(table_index) != 0) {
		msg(MSG_ERROR,"IpfixDbReader: Could not get columns for template");
		return 1;
	}

	for(i = 0; i < dbData->colCount; i++) {			
		dataTemplateInfo->fieldCount++;
		dataTemplateInfo->fieldInfo = (IpfixRecord::FieldInfo*)realloc(dataTemplateInfo->fieldInfo,
						      sizeof(IpfixRecord::FieldInfo)*dataTemplateInfo->fieldCount);
		IpfixRecord::FieldInfo* fi = &dataTemplateInfo->fieldInfo[dataTemplateInfo->fieldCount - 1];	
		fi->type.id = dbData->columns[i]->ipfixId;
		fi->type.length = dbData->columns[i]->length;
		fi->type.eid = 0;
		fi->offset = fieldLength;
		fieldLength = fieldLength + fi->type.length; 
	}

	/* Pass Data Template to flowSinks */
	boost::shared_ptr<IpfixDataTemplateRecord> ipfixRecord(new IpfixDataTemplateRecord);
	ipfixRecord->sourceID = srcId;
	ipfixRecord->dataTemplateInfo = dataTemplateInfo;
	push(ipfixRecord);
	msg(MSG_DEBUG,"IpfixDbReader sent template for table %s", dbData->tableNames[table_index]);
	return 0;
}


void copyUintNetByteOrder(IpfixRecord::Data* dest, char* src, IpfixRecord::FieldInfo::Type type) {
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

int IpfixDbReader::dbReaderSendTable(boost::shared_ptr<IpfixRecord::DataTemplateInfo> dataTemplateInfo, int table_index)
{
       	MYSQL_RES* dbResult = NULL;
	MYSQL_ROW dbRow = NULL;
	DbData* dbData = dbReader->dbData;
	int i;
	boost::shared_array<IpfixRecord::Data> data(new IpfixRecord::Data[MAX_MSG_LEN]);
	int dataLength = 0;
	unsigned delta = 0;
	unsigned flowTime = 0;
	unsigned lastFlowTime = 0;
	long long tmp;
	

	char select[STARTLEN] = "SELECT * FROM ";
	strncat(select, dbReader->dbData->tableNames[table_index],TABLE_WIDTH);
	strcat(select," ORDER BY lastSwitched");
	/** get all data from database*/
	if(mysql_query(conn, select) != 0) {
		msg(MSG_ERROR,"IpfixDbReader: Select on table failed. Error: %s",
		    mysql_error(conn));
		return 1;
	}

	dbResult = mysql_store_result(conn);
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
				return 1;
			}
		}
		for(i = 0; i != dbData->colCount; ++i) {
			switch(dbData->columns[i]->ipfixId) {
			case IPFIX_TYPEID_flowEndSeconds:
			        flowTime = atoll(dbRow[i]) + delta;
			case IPFIX_TYPEID_flowStartSeconds:
				tmp = atoll(dbRow[i]) + delta;
				copyUintNetByteOrder(data.get() + dataTemplateInfo->fieldInfo[i].offset,
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
				copyUintNetByteOrder(data.get() + dataTemplateInfo->fieldInfo[i].offset,
						     (char*)&tmp,
						     dataTemplateInfo->fieldInfo[i].type);
				dataLength += dataTemplateInfo->fieldInfo[i].type.length;
				break;
			}
		}
	
		/** according to flowstarttime wait for sending the record*/
		if(flowTime != lastFlowTime) {
			time_t t = time(NULL);
			if (t > (int)flowTime) {
				msg(MSG_ERROR, "IpfixDbReader: Sending flows too slowly");
			} else {
				sleep (flowTime - t);
			}
			lastFlowTime = flowTime;
		}



		boost::shared_ptr<IpfixDataDataRecord> ipfixRecord(new IpfixDataDataRecord);
		ipfixRecord->sourceID = srcId;
		ipfixRecord->dataTemplateInfo = dataTemplateInfo;
		ipfixRecord->dataLength = dataLength;
		ipfixRecord->message = data;
		ipfixRecord->data = data.get();
		push(ipfixRecord);
		msg(MSG_INFO,"IpfixDbReader sent record");
	}
	mysql_free_result(dbResult);
	
	msg(MSG_DEBUG,"Sending from table %s done", dbData->tableNames[table_index]);

	return 0;
}


/**
 * get all tableNames in database that matches with the wildcard "h%"
 **/
int IpfixDbReader::dbReaderDestroyTemplate(boost::shared_ptr<IpfixRecord::DataTemplateInfo> dataTemplateInfo)
{
	boost::shared_ptr<IpfixDataTemplateDestructionRecord> ipfixRecord(new IpfixDataTemplateDestructionRecord);
	ipfixRecord->sourceID = srcId;
	ipfixRecord->dataTemplateInfo = dataTemplateInfo;
	push(ipfixRecord);
	msg(MSG_DEBUG,"IpfixDbReader destroyed template");

	return 0;
}




/**
 * get all tableNames in database that matches with the wildcard "h%"
 **/
int IpfixDbReader::getTables()
{
	DbData* dbData = dbReader->dbData;
	int i = 0;
	char* wild = "h%";
	MYSQL_RES* dbResult = NULL;
	MYSQL_ROW dbRow = NULL;
	
	dbResult = mysql_list_tables(conn, wild);
	if(dbResult == 0) {
		msg(MSG_FATAL,"There are no flow tables in database %s", dbName);	
		return 1;
	}
	
	if((int)mysql_num_rows(dbResult) > MAX_TABLES) {
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

IpfixDbReader::columnDB* IpfixDbReader::getColumnByName(const char* name)
{
	static IpfixDbReader::columnDB tabs[] = {		 
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
	};

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
int IpfixDbReader::getColumns(int table_index)
{
	DbData* dbData = dbReader->dbData;
	MYSQL_RES* dbResult = NULL;
	MYSQL_ROW dbRow = NULL;
	
	char showcolStr[STARTLEN] = "SHOW COLUMNS FROM ";
	/* get column names from table table_index */
	strncat(showcolStr, dbData->tableNames[table_index],strlen(dbData->tableNames[table_index])+1);
	if(mysql_query(conn, showcolStr) != 0) {	
		msg(MSG_ERROR,"Show columns on table %s failed. Error: %s",
		    mysql_error(conn));
		return 1;
	}
	
	dbResult = mysql_store_result(conn);
	
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


int IpfixDbReader::connectToDb(
		const char* hostName, const char* dbName, 
		const char* userName, const char* password,
		unsigned int port, uint16_t observationDomainId)

{
	/** get the mysl init handle*/
	conn = mysql_init(0); 
	if(conn == 0) {
		msg(MSG_FATAL,"Get MySQL connect handle failed. Error: %s",
		    mysql_error(conn));
		return 1;
	} else {
		msg(MSG_DEBUG,"mysql init successfull");
	}
	/**Initialize structure members IpfixDbWriter*/
	this->hostName = hostName;
	this->dbName = dbName;
	this->userName = userName;
	this->password = password;
	this->portNum = port;
	this->socketName = 0;	  		
	this->flags = 0;
	srcId.reset(new IpfixRecord::SourceID);
	srcId->observationDomainId = observationDomainId;
	/**Initialize structure members DbData*/
	dbReader->dbData->colCount = 0;
	dbReader->dbData->tableCount = 0;
	
	/**Connect to Database*/
	if (!mysql_real_connect(conn,
				hostName,
				userName,password,
				0, portNum, socketName,
				flags)) {
		msg(MSG_FATAL,"Connection to database failed. Error: %s",
		    mysql_error(conn));
		return 1;
	}

	return 0;
}

/***** Exported Functions ****************************************************/

/**
 * Starts or resumes database
 * @param ipfixDbReader handle obtained by calling @c createipfixDbReader()
 */
int IpfixDbReader::start() {
	pthread_mutex_unlock(&mutex);
	return 0;
}

/**
 * Temporarily pauses database
 * @param ipfixDbReader handle obtained by calling @c createipfixDbReader()
 */
int IpfixDbReader::stop() {
	pthread_mutex_lock(&mutex);
	return 0;
}

/**
 * Frees memory used by an ipfixDbReader
 * @param ipfixDbWriter handle obtained by calling @c createipfixDbReader()
 */
IpfixDbReader::~IpfixDbReader() {
	mysql_close(conn);
	if (!pthread_mutex_destroy(&mutex)) {
		msg(MSG_ERROR, "Could not destroy mutex");
	}
	free(dbReader->dbData);
	free(dbReader);
}

/**
 * Creates a new ipfixDbReader. Do not forget to call @c startipfixDbReader() to begin reading from Database
 * @return handle to use when calling @c destroyipfixDbRreader()
 */
IpfixDbReader::IpfixDbReader(const char* hostName, const char* dbName, 
				   const char* userName, const char* password,
				   unsigned int port, uint16_t observationDomainId)
{
	DbData* dbData;

	if (pthread_mutex_init(&mutex, NULL)) {
		msg(MSG_FATAL, "Could not init mutex");
		goto out1;
	}

        if (pthread_mutex_lock(&mutex)) {
                msg(MSG_FATAL, "Could not lock mutex");
                goto out1;
        }

	dbReader = (DbReader*)malloc(sizeof(DbReader));
	if (!dbReader) {
		msg(MSG_ERROR, "Could not allocate DbReader");
		goto out1;
	}

	dbData = (DbData*)malloc(sizeof(DbData));
	if (!dbData) {
		msg(MSG_ERROR, "Could not allocate dbData");
		goto out2;
	}
	
	dbReader->dbData = dbData;
	if (connectToDb(hostName, dbName, userName,
			password, port, observationDomainId)) {
		goto out3;
	}
	msg(MSG_DEBUG,"Connected to database");
	
	/** use database  with db_name**/	
	if(mysql_select_db(conn, dbName) !=0) {
		msg(MSG_FATAL,"Database %s not selectable", dbName);	
		goto out3;
	} else {
		msg(MSG_DEBUG,"Database %s selected", dbName);
	}
	/** get tableNames of the database*/
	if(getTables() != 0) {
		msg(MSG_ERROR,"Error in function getTables");
		goto out3;
	}
	/**initialize columns**/
	dbData->colCount = 0;

	if (pthread_create(&thread, 0, readFromDB, this)) {
		msg(MSG_FATAL, "Could not create dbRead thread");
                goto out3;
	}
	
	return;

out3:
	free(dbData);
out2:
	free(dbReader);
out1:
	THROWEXCEPTION("IpfixDbReader creation failed");
	return;
}

#endif
