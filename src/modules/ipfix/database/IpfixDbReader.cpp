/*
 * IPFIX Database Reader/Writer
 * Copyright (C) 2006 Jürgen Abberger
 * Copyright (C) 2006 Lothar Braun <braunl@informatik.uni-tuebingen.de>
 * Copyright (C) 2008 Gerhard Muenz
 * Copyright (C) 2009-2013 Vermont Project
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

#if defined(DB_SUPPORT_ENABLED) || defined(MONGO_SUPPORT_ENABLED) || defined(PG_SUPPORT_ENABLED) || defined(ORACLE_SUPPORT_ENABLED) || defined(REDIS_SUPPORT_ENABLED)

#include <stdexcept>
#include <string.h>
#include <stdlib.h>
#include "IpfixDbReader.hpp"
#include "IpfixDbCommon.hpp"
#include "common/msg.h"


InstanceManager<IpfixTemplateRecord> IpfixDbReader::templateRecordIM("DbReaderIpfixTemplateRecord", 1);
InstanceManager<IpfixDataRecord> IpfixDbReader::dataRecordIM("DbReaderIpfixDataRecord", 50);
InstanceManager<IpfixTemplateDestructionRecord> IpfixDbReader::templateDestructionRecordIM("DbReaderIpfixTemplateDestructionRecord", 1);

/***** Internal Functions ****************************************************/

void copyUintNetByteOrder(IpfixRecord::Data* dest, char* src, InformationElement::IeInfo type);

/**
 * First send a a new template, then send the dataTemplates for all stored
 * tables.
 */
void* IpfixDbReader::readFromDB(void* ipfixDbReader_)
{
	IpfixDbReader* ipfixDbReader = (IpfixDbReader*)ipfixDbReader_;

	ipfixDbReader->registerCurrentThread();

	msg(MSG_DIALOG, "IpfixDbReader: Start sending tables");
	for(vector<string>::iterator i = ipfixDbReader->tables.begin(); i != ipfixDbReader->tables.end() && !ipfixDbReader->exitFlag; i++) {
		boost::shared_ptr<TemplateInfo> templateInfo(new TemplateInfo);
		templateInfo->setId = TemplateInfo::IpfixTemplate;
		if(ipfixDbReader->dbReaderSendNewTemplate(templateInfo, *i) != 0)
		{
		    msg(MSG_ERROR, "IpfixDbReader: Template error, skip table");
		    continue;
		}
		ipfixDbReader->dbReaderSendTable(templateInfo, *i);
		ipfixDbReader->dbReaderDestroyTemplate(templateInfo);

	}

	ipfixDbReader->unregisterCurrentThread();
	
	msg(MSG_DIALOG,"IpfixDbReader: Sending from database is done");
	return 0;
}
/**
 * Constructs a template from the table data and sends it to all connected
 * modules.
 */
int IpfixDbReader::dbReaderSendNewTemplate(boost::shared_ptr<TemplateInfo> templateInfo, const string& tableName)
{
	// reset record length 
	recordLength  = 0;

	/**get columnsname of the table*/
	if(getColumns(tableName) != 0) {
		msg(MSG_ERROR,"IpfixDbReader: Could not get columns for template");
		return 1;
	}

	for(vector<struct ipfix_identifier>::iterator i = columns.begin(); i != columns.end(); i++) {			
		templateInfo->fieldCount++;
		templateInfo->fieldInfo = (TemplateInfo::FieldInfo*)realloc(templateInfo->fieldInfo,
						      sizeof(TemplateInfo::FieldInfo)*templateInfo->fieldCount);
		TemplateInfo::FieldInfo* fi = &templateInfo->fieldInfo[templateInfo->fieldCount - 1];	
		fi->type.id = i->id;
		fi->type.length = i->length;
		fi->type.enterprise = 0;
		fi->offset = recordLength;
		recordLength = recordLength + fi->type.length; 
	}

	/* Pass Data Template to flowSinks */
	IpfixTemplateRecord* ipfixRecord = templateRecordIM.getNewInstance();
	ipfixRecord->sourceID = srcId;
	ipfixRecord->templateInfo = templateInfo;
	send(ipfixRecord);
	msg(MSG_DEBUG,"IpfixDbReader: sent template for table %s", tableName.c_str());
	return 0;
}


void copyUintNetByteOrder(IpfixRecord::Data* dest, char* src, InformationElement::IeInfo type) {
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

int IpfixDbReader::dbReaderSendTable(boost::shared_ptr<TemplateInfo> templateInfo, const string& tableName)
{
	MYSQL_RES* dbResult = NULL;
	MYSQL_ROW dbRow = NULL;
	unsigned offset = 0;
	uint64_t delta = 0;		// 64 bit to avoid castings in the case of flowStartMilliseconds
	uint32_t flowTime = 0;		// in seconds, so 32 bit are sufficient
	uint32_t lastFlowTime = 0;
	uint64_t tmp;
	bool first = true; 
	unsigned j = 0;
	
	string query = "SELECT " + columnNames + " FROM " + tableName;
	
	msg(MSG_VDEBUG, "IpfixDbReader: SQL query: %s", query.c_str());
	if(mysql_query(conn, query.c_str()) != 0) {
		msg(MSG_ERROR,"IpfixDbReader: Select on table failed. Error: %s",
		    mysql_error(conn));
		return 1;
	}

	dbResult = mysql_store_result(conn);
	msg(MSG_INFO,"IpfixDbReader:  Start sending records from table %s", tableName.c_str());

	while((dbRow = mysql_fetch_row(dbResult)) && !exitFlag) {
		// build new record
		boost::shared_array<IpfixRecord::Data> data(new IpfixRecord::Data[recordLength]);
		offset = 0;
		j = 0;
		for(vector<struct ipfix_identifier>::iterator i = columns.begin(); i != columns.end(); ++i) {
			tmp = atoll(dbRow[j]);
			copyUintNetByteOrder(data.get() + templateInfo->fieldInfo[j].offset,
					     (char*)&tmp,
					     templateInfo->fieldInfo[j].type);
			offset += templateInfo->fieldInfo[j].type.length;
			j++;
		}

		IpfixDataRecord* ipfixRecord = dataRecordIM.getNewInstance();
		ipfixRecord->sourceID = srcId;
		ipfixRecord->templateInfo = templateInfo;
		ipfixRecord->dataLength = offset; // = recordLength
		ipfixRecord->message = data;
		ipfixRecord->data = data.get();
		send(ipfixRecord);
		msg(MSG_VDEBUG,"IpfixDbReader: Record sent");
	}
	mysql_free_result(dbResult);
	
	if(!exitFlag)
		msg(MSG_INFO,"IpfixDbReader: Sending from table %s done", tableName.c_str());
	else
		msg(MSG_INFO,"IpfixDbReader: Sending from table %s aborted", tableName.c_str());

	return 0;
}


int IpfixDbReader::dbReaderDestroyTemplate(boost::shared_ptr<TemplateInfo> templateInfo)
{
	IpfixTemplateDestructionRecord* ipfixRecord = templateDestructionRecordIM.getNewInstance();
	ipfixRecord->sourceID = srcId;
	ipfixRecord->templateInfo = templateInfo;
	send(ipfixRecord);
	msg(MSG_DEBUG,"IpfixDbReader: Template destroyed");

	return 0;
}


/**
 * get all tables in database that matches with the wildcard "h\_%"
 **/
int IpfixDbReader::getTables()
{
	const char* wild = "f\\_%";
	MYSQL_RES* dbResult = NULL;
	MYSQL_ROW dbRow = NULL;

	dbResult = mysql_list_tables(conn, wild);
	if(dbResult == 0) {
		msg(MSG_FATAL,"IpfixDbReader: There are no flow tables in database");	
	} else {
		while((dbRow = mysql_fetch_row(dbResult))) {
			tables.push_back(string(dbRow[0]));
			msg(MSG_FATAL, "%s", dbRow[0]);
			msg(MSG_VDEBUG, "IpfixDbReader: table %s", tables.back().c_str());
		}
	}

	mysql_free_result(dbResult);
	
	return 0;
}


/**
 * Get the names of columns
 */
int IpfixDbReader::getColumns(const string& tableName)
{
	MYSQL_RES* dbResult = NULL;
	MYSQL_ROW dbRow = NULL;
	
	string query = "SHOW COLUMNS FROM " + tableName;
	msg(MSG_VDEBUG, "IpfixDbReader: SQL query: %s", query.c_str());
	if(mysql_query(conn, query.c_str()) != 0) {	
		msg(MSG_ERROR,"IpfixDbReader: Show columns on table %s failed. Error: %s",
			tableName.c_str(), mysql_error(conn));
		return 1;
	}
	
	dbResult = mysql_store_result(conn);
	
	if(dbResult == 0) {
		msg(MSG_FATAL,"IpfixDbReader: There are no Columns in the table");	
		return 1;
	}
	
	// TODO: don't we have to free the result of mysql_fetch_row?????
	columns.clear();
	columnNames = "";
	orderBy = ""; 
	while((dbRow = mysql_fetch_row(dbResult))) {
		bool found = true;
		const struct ipfix_identifier* id = ipfix_name_lookup(dbRow[0]);
		if (id == NULL) {
			msg(MSG_INFO, "IpfixDbReader: Unsupported column: %s", dbRow[0]);
		} else {
			columnNames = columnNames + "," + dbRow[0];
			columns.push_back(*id);
			msg(MSG_VDEBUG, "IpfixDbReader: column %s (%d)", dbRow[0], columns.back().id);
		}
	}
	
	// remove leading ","
	if(columnNames != "")
		columnNames.erase(0,1);

	mysql_free_result(dbResult);

	return 0;
}


int IpfixDbReader::connectToDb(
		const string& hostName, const string& dbName, 
		const string& userName, const string& password,
		unsigned int port)
{
	/** get the mysl init handle*/
	conn = mysql_init(0); 
	if(conn == 0) {
		msg(MSG_FATAL,"IpfixDbReader: Get  connect handle failed. Error: %s",
		    mysql_error(conn));
		return 1;
	} else {
		msg(MSG_DEBUG,"IpfixDbReader: mysql init successful");
	}

	/**Connect to Database*/
	if (!mysql_real_connect(conn, hostName.c_str(), userName.c_str(),password.c_str(),
				0, port, 0, 0)) {
		msg(MSG_FATAL,"IpfixDbReader: Connection to database failed. Error: %s",
		    mysql_error(conn));
		return 1;
	} else {
		msg(MSG_DEBUG,"IpfixDbReader: successfully connected to database");
	}

	/** use database with dbName **/	
	if(mysql_select_db(conn, dbName.c_str()) !=0) {
		msg(MSG_FATAL,"IpfixDbReader: Database %s not selectable", dbName.c_str());	
		return 1;
	} else {
		msg(MSG_DEBUG,"IpfixDbReader: Database %s selected", dbName.c_str());
	}

	return 0;
}

/***** Exported Functions ****************************************************/

/**
 * Starts or resumes database
 * @param ipfixDbReader handle obtained by calling @c createipfixDbReader()
 */
void IpfixDbReader::performStart() 
{
	thread.run(this);
}

/**
 * Temporarily pauses database
 * @param ipfixDbReader handle obtained by calling @c createipfixDbReader()
 */
void IpfixDbReader::performShutdown() 
{
	thread.join();
}

/**
 * Frees memory used by an ipfixDbReader
 * @param ipfixDbWriter handle obtained by calling @c createipfixDbReader()
 */
IpfixDbReader::~IpfixDbReader() {
	mysql_close(conn);
}

/**
 * Creates a new ipfixDbReader. Do not forget to call @c startipfixDbReader() to begin reading from Database
 * @return handle to use when calling @c destroyipfixDbRreader()
 */
IpfixDbReader::IpfixDbReader(const string& dbType, const string& hostname, const string& dbname,
				const string& username, const string& password,
				unsigned port, uint16_t observationDomainId)
	: thread(readFromDB) 
{
	srcId.reset(new IpfixRecord::SourceID);
	srcId->observationDomainId = observationDomainId;
	srcId->exporterAddress.len = 0;
	srcId->exporterPort = 0;
	srcId->receiverPort = 0;
	srcId->protocol = 0;
	srcId->fileDescriptor = 0;

	if (connectToDb(hostname, dbname, username, password, port)) {
		THROWEXCEPTION("IpfixDbReader creation failed");
	}

	/** get tables of the database*/
	if(getTables() != 0) {
		msg(MSG_ERROR,"IpfixDbReader: Error in function getTables");
		THROWEXCEPTION("IpfixDbReader creation failed");
	}
}

#endif
