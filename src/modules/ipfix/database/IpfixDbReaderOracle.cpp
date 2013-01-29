/*
 * IPFIX Database Reader for Oracle DBs
 * Copyright (C) 2006 Jürgen Abberger
 * Copyright (C) 2006-2012 Lothar Braun
 * Copyright (C) 2008 Gerhard Muenz
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

#ifdef ORACLE_SUPPORT_ENABLED

#include <stdexcept>
#include <string.h>
#include <stdlib.h>
#include <sstream>
#include "IpfixDbReaderOracle.hpp"
#include "IpfixDbCommon.hpp"
#include "common/msg.h"


/***** Internal Functions ****************************************************/

int IpfixDbReaderOracle::dbReaderSendTable(boost::shared_ptr<TemplateInfo> templateInfo, const string& tableName)
{
	std::ostringstream sql;
	oracle::occi::Statement *stmt = NULL;
	oracle::occi::ResultSet *rs = NULL;

	uint64_t tmp;
	bool first = true; 
	unsigned offset = 0;
	unsigned j = 0;

	msg(MSG_VDEBUG, "IpfixDbReaderOracle: Sending table %s", tableName.c_str());
	
	sql << "SELECT " << columnNames << " FROM "<< tableName;

	// create the oracle statement
	try {
		stmt = con->createStatement(sql.str());
	} catch (oracle::occi::SQLException& ex) {
		msg(MSG_FATAL, "IpfixDbReaderOracle: Error creating statement: %s", ex.getMessage().c_str());
		return 1;
	}
	
	try {
		stmt->setPrefetchRowCount(1);
		rs = stmt->executeQuery();
	} catch (oracle::occi::SQLException& ex) {
		msg(MSG_FATAL,"IpfixDbReaderOracle: Error executing statement: %s", ex.getMessage().c_str());
		con->terminateStatement(stmt);
	}

	if (!rs) {
		msg(MSG_ERROR, "IpfixDbReaderOracle: Table %s was empty!", tableName.c_str());
		return 1;
	}

	msg(MSG_INFO,"IpfixDbReaderOracle:  Start sending records from table %s", tableName.c_str());

	try {
	while((rs->next()) && !exitFlag) {
		// build new record
		boost::shared_array<IpfixRecord::Data> data(new IpfixRecord::Data[recordLength]);
		offset = 0;
		j = 0;
		for(vector<struct ipfix_identifier>::iterator i = columns.begin(); i != columns.end(); ++i) {
			// do not forget: Oracle starts counting at 1
			tmp = atoll(rs->getString(j + 1).c_str());
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
	}
	stmt->closeResultSet(rs);
	con->terminateStatement(stmt);
	
	} catch (oracle::occi::SQLException& ex) {
                msg(MSG_ERROR, "Caught SQL exception while getting flows from table: %s", ex.getMessage().c_str());
		return 1;
	}

	if(!exitFlag)
		msg(MSG_INFO,"IpfixDbReaderOracle: Sending from table %s done", tableName.c_str());
	else
		msg(MSG_INFO,"IpfixDbReaderOracle: Sending from table %s aborted", tableName.c_str());

	return 0;
}

/**
 * get all tables in database that matches with the wildcard "h\_%"
 **/
int IpfixDbReaderOracle::getTables()
{
	std::ostringstream sql;
        oracle::occi::Statement *stmt = NULL;
        oracle::occi::ResultSet *rs = NULL;


        sql << "SELECT table_name FROM user_tables WHERE table_name LIKE 'F_%' ORDER BY table_name ASC";
        // create the oracle statement
        try {
                stmt = con->createStatement(sql.str());
        } catch (oracle::occi::SQLException& ex) {
                msg(MSG_FATAL, "IpfixDbReaderOracle: Error creating statement: %s", ex.getMessage().c_str());
                return 1;
        }

        //msg(MSG_VDEBUG, "IpfixDbReaderOracle: SQL query: %s", query.c_str());
        try {
                stmt->setPrefetchRowCount(1);
                rs = stmt->executeQuery();
        } catch (oracle::occi::SQLException& ex) {
                msg(MSG_FATAL,"IpfixDbWriterOracle: Error executing statement: %s", ex.getMessage().c_str());
                con->terminateStatement(stmt);
        }

        if (!rs) {
                msg(MSG_ERROR, "IpfixDbWriterOracle: Found no flow tables!");
                return 1;
        }

	try {
        while((rs->next()) && !exitFlag) {
		tables.push_back(rs->getString(1));
		msg(MSG_VDEBUG, "IpfixDbReaderOracle: table %s", tables.back().c_str());
	}

        stmt->closeResultSet(rs);
        con->terminateStatement(stmt);

        } catch (oracle::occi::SQLException& ex) {
                msg(MSG_ERROR, "Caught SQL exception: %s", ex.getMessage().c_str());
                return 1;
        }
	
	return 0;
}


/**
 * Get the names of columns
 */
int IpfixDbReaderOracle::getColumns(const string& tableName)
{
        oracle::occi::Statement *stmt = NULL;
        oracle::occi::ResultSet *rs = NULL;

	string query = "SELECT column_name FROM cols WHERE table_name = '" + tableName + "'";
        // create the oracle statement
        try {
                stmt = con->createStatement(query);
        } catch (oracle::occi::SQLException& ex) {
                msg(MSG_FATAL, "IpfixDbReaderOracle: Error creating statement: %s", ex.getMessage().c_str());
                return 1;
        }

        //msg(MSG_VDEBUG, "IpfixDbReaderOracle: SQL query: %s", query.c_str());
        try {
                stmt->setPrefetchRowCount(1);
                rs = stmt->executeQuery();
        } catch (oracle::occi::SQLException& ex) {
                msg(MSG_FATAL,"IpfixDbWriterOracle: Error executing statement: %s", ex.getMessage().c_str());
                con->terminateStatement(stmt);
        }

        if (!rs) {
                msg(MSG_ERROR, "IpfixDbWriterOracle: Flow tables do not have columns??");
                return 1;
        }

	columns.clear();
	columnNames = "";
	orderBy = ""; 
	try {
        while((rs->next()) && !exitFlag) {
		bool found = true;
		const struct ipfix_identifier* id = ipfix_name_lookup(rs->getString(1).c_str());
		if (id == NULL) {
			msg(MSG_INFO, "IpfixDbReaderMySQL: Unsupported column: %s", rs->getString(1).c_str());
		} else {
			columnNames = columnNames + "," + rs->getString(1).c_str();
			columns.push_back(*id);
			msg(MSG_VDEBUG, "IpfixDbReaderMySQL: column %s (%d)", rs->getString(1).c_str(), columns.back().id);
		}
		if(found)
			msg(MSG_VDEBUG, "IpfixDbReaderOracle: column %s (%d)", rs->getString(1).c_str(), columns.back().id);
	}
	
	if(columnNames != "")
		columnNames.erase(0,1);

        stmt->closeResultSet(rs);
        con->terminateStatement(stmt);
        } catch (oracle::occi::SQLException& ex) {
                msg(MSG_ERROR, "Caught SQL exception: %s", ex.getMessage().c_str());
                return 1;
        }

	return 0;
}


int IpfixDbReaderOracle::connectToDb()
{
	dbError = true;

	// try to close connection in case we still have one
	// from a previous try
	if (con) {
		env->terminateConnection(con);
	}	

	msg(MSG_DEBUG, "IpfixDbReaderOracle: Creating environment.");
	try {
		env = oracle::occi::Environment::createEnvironment(oracle::occi::Environment::DEFAULT);
	} catch (oracle::occi::SQLException& ex) {
		msg(MSG_FATAL, "IpfixDbReaderOracle: Error while creating environment: %s.", ex.getMessage().c_str());
		msg(MSG_FATAL, "IpfixDbReaderOracle: Did you configure your Oracle environment?");
		return -1;
	}
	msg(MSG_DEBUG, "IpfixDbReaderOracle: Trying to connect to database ...");
	try 
	{
		char dbLogon[256];
		sprintf(dbLogon, "%s:%u/%s", hostname.c_str(), port, dbname.c_str());
		con = env->createConnection(username, password, dbLogon);
	} catch (oracle::occi::SQLException& ex) 
	{
		msg(MSG_FATAL,"IpfixDbReaderOracle: Oracle connect failed. Error: %s", ex.getMessage().c_str());
		return 1;
	}
	msg(MSG_DEBUG,"IpfixDbReaderOracle: Successfully connected to Oracle DB");

	return 0;
}

/***** Exported Functions ****************************************************/

/**
 * Frees memory used by an ipfixDbReader
 * @param ipfixDbWriter handle obtained by calling @c createipfixDbReader()
 */
IpfixDbReaderOracle::~IpfixDbReaderOracle() {
	env->terminateConnection(con);
	oracle::occi::Environment::terminateEnvironment(env);
}

/**
 * Creates a new ipfixDbReader. Do not forget to call @c startipfixDbReader() to begin reading from Database
 * @return handle to use when calling @c destroyipfixDbRreader()
 */
IpfixDbReaderOracle::IpfixDbReaderOracle(const string& dbType, const string& Hostname, const string& Dbname,
				const string& Username, const string& Password,
				uint16_t Port, uint16_t ObservationDomainId)
	: IpfixDbReader(dbType, Hostname, Dbname, Username, Password, Port, ObservationDomainId)
{
}

#endif
