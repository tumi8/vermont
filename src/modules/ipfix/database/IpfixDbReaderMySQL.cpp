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

#if defined(DB_SUPPORT_ENABLED) 

#include <stdexcept>
#include <string.h>
#include <stdlib.h>
#include "IpfixDbReaderMySQL.hpp"
#include "IpfixDbCommon.hpp"
#include "common/msg.h"

/***** Internal Functions ****************************************************/

/**
 * Select a given table and get the values by reading
 * the database. The Typs of the values from database are
 * strings, therefore they must change into IPFIX format 
*/

int IpfixDbReaderMySQL::dbReaderSendTable(boost::shared_ptr<TemplateInfo> templateInfo, const string& tableName)
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
	
	msg(MSG_VDEBUG, "IpfixDbReaderMySQL: SQL query: %s", query.c_str());
	if(mysql_query(conn, query.c_str()) != 0) {
		msg(MSG_ERROR,"IpfixDbReaderMySQL: Select on table failed. Error: %s",
		    mysql_error(conn));
		return 1;
	}

	dbResult = mysql_store_result(conn);
	msg(MSG_INFO,"IpfixDbReaderMySQL:  Start sending records from table %s", tableName.c_str());

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
		msg(MSG_VDEBUG,"IpfixDbReaderMySQL: Record sent");
	}
	mysql_free_result(dbResult);
	
	if(!exitFlag)
		msg(MSG_INFO,"IpfixDbReaderMySQL: Sending from table %s done", tableName.c_str());
	else
		msg(MSG_INFO,"IpfixDbReaderMySQL: Sending from table %s aborted", tableName.c_str());

	return 0;
}

/**
 * get all tables in database that matches with the wildcard "h\_%"
 **/
int IpfixDbReaderMySQL::getTables()
{
	const char* wild = "f\\_%";
	MYSQL_RES* dbResult = NULL;
	MYSQL_ROW dbRow = NULL;

	dbResult = mysql_list_tables(conn, wild);
	if(dbResult == 0) {
		msg(MSG_FATAL,"IpfixDbReaderMySQL: There are no flow tables in database");	
	} else {
		while((dbRow = mysql_fetch_row(dbResult))) {
			tables.push_back(string(dbRow[0]));
			msg(MSG_VDEBUG, "IpfixDbReaderMySQL: table %s", tables.back().c_str());
		}
	}

	mysql_free_result(dbResult);
	
	return 0;
}


/**
 * Get the names of columns
 */
int IpfixDbReaderMySQL::getColumns(const string& tableName)
{
	MYSQL_RES* dbResult = NULL;
	MYSQL_ROW dbRow = NULL;
	
	string query = "SHOW COLUMNS FROM " + tableName;
	msg(MSG_VDEBUG, "IpfixDbReaderMySQL: SQL query: %s", query.c_str());
	if(mysql_query(conn, query.c_str()) != 0) {	
		msg(MSG_ERROR,"IpfixDbReaderMySQL: Show columns on table %s failed. Error: %s",
			tableName.c_str(), mysql_error(conn));
		return 1;
	}
	
	dbResult = mysql_store_result(conn);
	
	if(dbResult == 0) {
		msg(MSG_FATAL,"IpfixDbReaderMySQL: There are no Columns in the table");	
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
			msg(MSG_INFO, "IpfixDbReaderMySQL: Unsupported column: %s", dbRow[0]);
		} else {
			columnNames = columnNames + "," + dbRow[0];
			columns.push_back(*id);
			msg(MSG_VDEBUG, "IpfixDbReaderMySQL: column %s (ID: %d, PEN: %u)", dbRow[0], columns.back().id, columns.back().pen);
		}
	}
	
	// remove leading ","
	if(columnNames != "")
		columnNames.erase(0,1);

	mysql_free_result(dbResult);

	return 0;
}


int IpfixDbReaderMySQL::connectToDb()
{
	/** get the mysl init handle*/
	conn = mysql_init(0); 
	if(conn == 0) {
		msg(MSG_FATAL,"IpfixDbReaderMySQL: Get  connect handle failed. Error: %s",
		    mysql_error(conn));
		return 1;
	} else {
		msg(MSG_DEBUG,"IpfixDbReaderMySQL: mysql init successful");
	}

	/**Connect to Database*/
	if (!mysql_real_connect(conn, hostname.c_str(), username.c_str(),password.c_str(),
				0, port, 0, 0)) {
		msg(MSG_FATAL,"IpfixDbReaderMySQL: Connection to database failed. Error: %s",
		    mysql_error(conn));
		return 1;
	} else {
		msg(MSG_DEBUG,"IpfixDbReaderMySQL: successfully connected to database");
	}

	/** use database with dbName **/	
	if(mysql_select_db(conn, dbname.c_str()) !=0) {
		msg(MSG_FATAL,"IpfixDbReaderMySQL: Database %s not selectable", dbname.c_str());	
		return 1;
	} else {
		msg(MSG_DEBUG,"IpfixDbReaderMySQL: Database %s selected", dbname.c_str());
	}

	return 0;
}

IpfixDbReaderMySQL::~IpfixDbReaderMySQL() {
	mysql_close(conn);
}

/**
 * Creates a new ipfixDbReader. Do not forget to call @c startipfixDbReader() to begin reading from Database
 * @return handle to use when calling @c destroyipfixDbRreader()
 */
IpfixDbReaderMySQL::IpfixDbReaderMySQL(const string& dbType, const string& Hostname, const string& Dbname,
				const string& Username, const string& Password,
				uint16_t Port, uint16_t ObservationDomainId)
	: IpfixDbReader(dbType, Hostname, Dbname, Username, Password, Port, ObservationDomainId)
{
}

#endif
