/*
 * IPFIX Database Reader/Writer Oracle Connector
 * Copyright (C) 2011 Philipp Fehre <philipp.fehre@googlemail.com>
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
#include <iomanip>
#include <stdlib.h>
#include "IpfixDbWriterOracle.hpp"
#include "common/msg.h"

/**
 * (re)connect to database
 */
void IpfixDbWriterOracle::connectToDB()
{
	dbError = true;
	
	// close (in the case that it was already connected)
	if (con) env->terminateConnection(con);

	/** get the initial environment and connect */
	msg(MSG_DEBUG, "IpfixDbWriterOracle: Creating environment.");
	try {
		env = oracle::occi::Environment::createEnvironment(oracle::occi::Environment::DEFAULT);
	} catch (oracle::occi::SQLException& ex) {
		msg(MSG_FATAL, "IpfixDbWriterOracle: Error while creating environment: %s.", ex.getMessage().c_str());
		msg(MSG_FATAL, "IpfixDbWriterOracle: Did you configure your Oracle environment?");
		return ;
	}
	msg(MSG_DEBUG, "IpfixDbWriterOracle: Trying to connect to database ...");
	try 
	{
		char dbLogon[256];
		sprintf(dbLogon, "%s:%u/%s", hostName, portNum, dbName);
		con = env->createConnection(userName, password, dbLogon);
	} catch (oracle::occi::SQLException& ex) 
	{
		msg(MSG_FATAL,"IpfixDbWriterOracle: Oracle connect failed. Error: %s", ex.getMessage().c_str());
		return ;
	}
	msg(MSG_DEBUG,"IpfixDbWriterOracle: Oracle connection successful");
	
	if (createExporterTable()!=0) return ;
	
	dbError = false;
	
}

std::string IpfixDbWriterOracle::insertRowPrefix()
{
	ostringstream sql;
	sql  << "INTO " << curTable.name << " (";
	for (uint32_t i = 0; i < numberOfColumns; ++i) {
		sql << tableColumns[i].cname;
		if (i < numberOfColumns - 1) sql << ",";
	}
	sql << ") VALUES ";
	return sql.str();
}

int IpfixDbWriterOracle::createExporterTable()
{
	// check if table exists
	ostringstream sql;
	oracle::occi::Statement *stmt = NULL;
	oracle::occi::ResultSet *rs = NULL;
	sql << "SELECT COUNT(table_name) FROM user_tables WHERE table_name='EXPORTER'";
	try
	{
		stmt = con->createStatement(sql.str());
	}
	catch (oracle::occi::SQLException& ex)
	{
		msg(MSG_FATAL,"IpfixDbWriterOracle: Error creating statement: %s", ex.getMessage().c_str());	
		dbError = true;
		return 1;		
	}
	if (stmt)
	{
		try 
		{
			stmt->setPrefetchRowCount(1);
			rs = stmt->executeQuery();			
		}
		catch (oracle::occi::SQLException& ex)
		{
			msg(MSG_FATAL,"IpfixDbWriterOracle: Error executing create exporter table: %s", ex.getMessage().c_str());	
			con->terminateStatement(stmt);
			dbError = true;
			return 1;					
		}
		if (rs)
		{
			while(rs->next())
			{
				if (rs->getInt(1)!= 0) 
				{
					msg(MSG_DEBUG,"IpfixDbWriterOracle: exporter table does exist");
					stmt->closeResultSet(rs);
					con->terminateStatement(stmt);
					return 0;	
				}
			}
			stmt->closeResultSet(rs);
		}
		con->terminateStatement(stmt);
	}

	// create table
	sql.str("");	
	sql << "CREATE TABLE exporter ( id NUMERIC(10) NOT NULL, sourceID NUMERIC(10), srcIP NUMERIC(10), CONSTRAINT exporter_pk PRIMARY KEY (id) )"; 
	try
	{
		stmt = con->createStatement(sql.str());
	}
	catch (oracle::occi::SQLException& ex)
	{
		msg(MSG_FATAL,"IpfixDbWriterOracle: Error creating exporter table statement: %s", ex.getMessage().c_str());	
		dbError = true;
		return 1;		
	}
	if (stmt)
	{
		try 
		{
			stmt->setPrefetchRowCount(1);
			rs = stmt->executeQuery();			
		}
		catch (oracle::occi::SQLException& ex)
		{
			msg(MSG_FATAL,"IpfixDbWriterOracle: Error creating exporter table: %s", ex.getMessage().c_str());	
			con->terminateStatement(stmt);
			dbError = true;
			return 1;					
		}
		msg(MSG_DEBUG,"IpfixDbWriterOracle: exporter table created");
		stmt->closeResultSet(rs);
		con->terminateStatement(stmt);
	}
	
	// create counter
	// clear vars for reuse
	sql.str("");	
	sql << "CREATE sequence counter_for_exporter increment BY 1 start WITH 1 cache 2";
	try
	{
		stmt = con->createStatement(sql.str());
	}
	catch (oracle::occi::SQLException& ex)
	{
		msg(MSG_FATAL,"IpfixDbWriterOracle: Error creating sequence counter statement: %s", ex.getMessage().c_str());	
		dbError = true;
		return 1;		
	}
	if (stmt)
	{
		try 
		{
			stmt->setPrefetchRowCount(1);
			rs = stmt->executeQuery();			
		}
		catch (oracle::occi::SQLException& ex)
		{
			msg(MSG_FATAL,"IpfixDbWriterOracle: Error creating squence counter table:  %s", ex.getMessage().c_str());	
			con->terminateStatement(stmt);
			dbError = true;
			return 1;					
		}
		msg(MSG_DEBUG,"IpfixDbWriterOracle: exporter table counter created");
		stmt->closeResultSet(rs);
		con->terminateStatement(stmt);
	}

	// create trigger
	sql.str("");	
	sql << "CREATE OR REPLACE TRIGGER trigger_for_id_exporter BEFORE INSERT ON exporter REFERENCING NEW AS new FOR EACH ROW Begin SELECT counter_for_exporter.NEXTVAL INTO :new.id FROM DUAL; End;";
	msg(MSG_DEBUG, "IpfixDbWriterOracle: SQL Query: %s", sql.str().c_str());
	try
	{
		stmt = con->createStatement(sql.str());
	}
	catch (oracle::occi::SQLException& ex)
	{
		msg(MSG_FATAL,"IpfixDbWriterOracle: Error creating statement: %s", ex.getMessage().c_str());	
		dbError = true;
		return 1;		
	}
	if (stmt)
	{
		try 
		{
			stmt->setPrefetchRowCount(1);
			rs = stmt->executeQuery();			
		}
		catch (oracle::occi::SQLException& ex)
		{
			msg(MSG_FATAL,"IpfixDbWriterOracle: Error executing trigger creation \"%s\": %s", sql.str().c_str(), ex.getMessage().c_str());	
			dbError = true;
			con->terminateStatement(stmt);
			return 1;					
		}
		msg(MSG_DEBUG,"IpfixDbWriterOracle: exporter table insert trigger created");
		stmt->closeResultSet(rs);
		con->terminateStatement(stmt);
	}	
	msg(MSG_DEBUG, "Exporter table creation done");
	return 0;
}


/*
 * Write insertStatement to database
 */
bool IpfixDbWriterOracle::writeToDb()
{
	if (insertBuffer.curRows == 0) {
		// nothing to write
		return 1;
	}

	
	// this is an insert operation. Oracle is a professional datatbase and therefore wants to have
	// some special handling of multi row inserts. Inserts should look like 
	// INSERT ALL
	//	INTO <tablename> (<column-spec>) VALUES (<values>)
	//	INTO <tablenaem> (<column-spec>) VALUES (<values>)
	//	...
	// SELECT * FROM dual;
	// we therefore need to append the dual select
	std::string sql_string = std::string(insertBuffer.sql) + " SELECT * FROM dual";

	oracle::occi::Statement *stmt = NULL;
	oracle::occi::ResultSet *rs = NULL;
	try
	{
		stmt = con->createStatement(sql_string.c_str());
	}
	catch (oracle::occi::SQLException& ex)
	{
		msg(MSG_FATAL,"IpfixDbWriterOracle: Error creating statement: %s", ex.getMessage().c_str());	
		dbError = true;
		return 0;		
	}
	if (stmt)
	{
		try 
		{
			stmt->setPrefetchRowCount(1);
			rs = stmt->executeQuery();			
		}
		catch (oracle::occi::SQLException& ex)
		{
			msg(MSG_FATAL,"IpfixDbWriterOracle: Error executing flow db insert \"%s\": %s", insertBuffer.sql, ex.getMessage().c_str());	
			dbError = true;
			con->terminateStatement(stmt);
			return 0;					
		}

		stmt->closeResultSet(rs);
		con->terminateStatement(stmt);

	}
	insertBuffer.curRows = 0;
	insertBuffer.appendPtr = insertBuffer.bodyPtr;
	*insertBuffer.appendPtr = 0;

	// commit transaction
	try {
		con->commit();
	} catch (oracle::occi::SQLException& ex) {
		dbError = true;
		msg(MSG_FATAL, "IpfixDbWriterOracle: Received exception during commit: \"%s\"", ex.getMessage().c_str());
	}

	return 1;
}

bool IpfixDbWriterOracle::createDBTable(const char* partitionname, uint64_t starttime, uint64_t endtime)
{

        uint32_t i;

        if (find(usedPartitions.begin(), usedPartitions.end(), partitionname)!=usedPartitions.end()) {
                // found cached entry!
                DPRINTF("Partition '%s' already created.", partitionname);
                return true;
        }

	// check if table exists
	ostringstream sql;
	oracle::occi::Statement *stmt = NULL;
	oracle::occi::ResultSet *rs = NULL;
	sql << "SELECT COUNT(table_name) FROM user_tables WHERE table_name='" << partitionname<< "'";
	msg(MSG_DEBUG, "IpfixDbWriterOracle: SQL Query: %s", sql.str().c_str());
	try
	{
		stmt = con->createStatement(sql.str());
	}
	catch (oracle::occi::SQLException& ex)
	{
		msg(MSG_FATAL,"IpfixDbWriterOracle: %s", ex.getMessage().c_str());	
		dbError = true;
		return 1;		
	}
	if (stmt)
	{
		try 
		{
			stmt->setPrefetchRowCount(1);
			rs = stmt->executeQuery();			
		}
		catch (oracle::occi::SQLException& ex)
		{
			msg(MSG_FATAL,"IpfixDbWriterOracle: %s", ex.getMessage().c_str());	
			con->terminateStatement(stmt);
			dbError = true;
			return 1;					
		}
		if (rs)
		{
			while(rs->next())
			{
				if (rs->getInt(1)!= 0) 
				{
					msg(MSG_DEBUG,"IpfixDbWriterOracle: table does exist");
					stmt->closeResultSet(rs);
					con->terminateStatement(stmt);
					return 0;	
				}
			}
			stmt->closeResultSet(rs);
		}
		con->terminateStatement(stmt);
	}

	// create table
	sql.str("");	
	sql << "CREATE TABLE " << partitionname<< " ( " << tableColumnsCreateString << ")";
	msg(MSG_DEBUG, "IpfixDbWriterOracle: SQL Query: %s", sql.str().c_str());
	try
	{
		stmt = con->createStatement(sql.str());
	}
	catch (oracle::occi::SQLException& ex)
	{
		msg(MSG_FATAL,"IpfixDbWriterOracle: Failed to prepare CREATE flow table statement \"%s: %s", sql.str().c_str(), ex.getMessage().c_str());	
		dbError = true;
		return 1;		
	}
	if (stmt)
	{
		try 
		{
			stmt->setPrefetchRowCount(1);
			rs = stmt->executeQuery();			
		}
		catch (oracle::occi::SQLException& ex)
		{
			msg(MSG_FATAL,"IpfixDbWriterOracle: Failed to execute CREATE flow statement \"%s\":  %s", sql.str().c_str(), ex.getMessage().c_str());	
			con->terminateStatement(stmt);
			dbError = true;
			return 1;					
		}
		msg(MSG_DEBUG,"IpfixDbWriterOracle: exporter table created");
		stmt->closeResultSet(rs);
		con->terminateStatement(stmt);
	}
	msg(MSG_DEBUG, "IpfixDbWriterOracle: Table %s created ", partitionname);
	return 0;
}

string IpfixDbWriterOracle::getInsertString(string tableName)
{
	// oracle multi-row insert statement differs from mysql/postgres multirow insert
	return "INSERT ALL ";
}


/**
 *	Returns the id of the exporter table entry or 0 in the case of an error
 */
int IpfixDbWriterOracle::getExporterID(IpfixRecord::SourceID* sourceID)
{
	uint32_t i;
	oracle::occi::Statement* stmt = NULL;
	oracle::occi::ResultSet* rs = NULL;
	int exporterID = -1;
	uint32_t expIp = 0;
	ostringstream sql;


	// convert IP address (correct host byte order since 07/2010)
	expIp = sourceID->exporterAddress.toUInt32();
	
	/** Is the exporterID already in exporterBuffer? */
	for(i = 0; i < curExporterEntries; i++) {
		if(exporterEntries[i].observationDomainId == sourceID->observationDomainId &&
			exporterEntries[i].ip==expIp) {
			DPRINTF("Exporter sourceID/IP with ID %d is in the exporterBuffer\n",
				exporterEntries[i].Id);
			return exporterEntries[i].Id;
		}
	}

	// search exporter table
	sql << "SELECT id FROM exporter WHERE sourceID=" << sourceID->observationDomainId << " AND srcIp=" << expIp;
	msg(MSG_DEBUG, "IpfixDbWriterOracle: SQL Query: %s", sql.str().c_str());
	try 
	{
		stmt = con->createStatement(sql.str());
	}
	catch (oracle::occi::SQLException &ex)
	{
 		msg(MSG_ERROR,"IpfixDbWriterOracle: Select on exporter table failed. Error: %s", ex.getMessage().c_str());	
		return 0;// If a failure occurs, return 0
	}
	if(stmt)
	{
		try 
		{
			stmt->setPrefetchRowCount(1);
			rs = stmt->executeQuery();
			if (rs)
			{
				while(rs->next())
				{
					exporterID = rs->getInt(1);
					msg(MSG_DEBUG, "IpfixDbWriterOracle: ExporterID %d is in exporter table", exporterID);					
				}
				stmt->closeResultSet(rs);
			}
			con->terminateStatement(stmt);
		}
		catch (oracle::occi::SQLException &ex)
		{
			msg(MSG_ERROR,"IpfixDbWriterOracle: Select on exporter table failed. Error: %s", ex.getMessage().c_str());	
			con->terminateStatement(stmt);
			dbError = true;
			return 0;// If a failure occurs, return 0			
		}
	}
	// insert new entry in exporter table since it is not found
	if(exporterID == -1)
	{
		sql.str("");
		sql << "INSERT INTO exporter (sourceID,srcIP) VALUES ('" << sourceID->observationDomainId << "','" << expIp << "')";
		msg(MSG_DEBUG, "IpfixDbWriterOracle: SQL Query: %s", sql.str().c_str());
		try
		{
			stmt = con->createStatement(sql.str());
		}
		catch (oracle::occi::SQLException& ex)
		{
			msg(MSG_ERROR,"IpfixDbWriterOracle: Insert in exporter table failed. Error: %s", ex.getMessage().c_str());	
			dbError = true;
			return 0;		
		}
		if (stmt)
		{
			try 
			{
				stmt->setPrefetchRowCount(1);
				rs = stmt->executeQuery();			
			}
			catch (oracle::occi::SQLException& ex)
			{
				msg(MSG_FATAL,"IpfixDbWriterOracle: Insert in exporter table failed. Error: %s", ex.getMessage().c_str());	
				dbError = true;
				con->terminateStatement(stmt);
				return 0;					
			}
			stmt->closeResultSet(rs);
			con->terminateStatement(stmt);
		}

		sql.str("");
		sql << "SELECT counter_for_exporter.CURRVAL FROM DUAL";
		msg(MSG_DEBUG, "IpfixDbWriterOracle: SQL Query: %s", sql.str().c_str());
		try 
		{
			stmt = con->createStatement(sql.str());
		}
		catch (oracle::occi::SQLException &ex)
		{
			msg(MSG_ERROR,"IpfixDbWriterOracle: Select on counter_for_exporter sequence failed. Error: %s", ex.getMessage().c_str());	
			dbError = true;
			return 0;// If a failure occurs, return 0
		}
		if(stmt)
		{
			try 
			{
				stmt->setPrefetchRowCount(1);
				rs = stmt->executeQuery();
				if (rs)
				{
					while(rs->next())
					{
						exporterID = rs->getInt(1);
						DPRINTF("ExporterID %d is in exporter table", exporterID);
					}
					stmt->closeResultSet(rs);
				}
				con->terminateStatement(stmt);
			}
			catch (oracle::occi::SQLException &ex)
			{
				msg(MSG_ERROR,"IpfixDbWriterOracle: Select on counter_for_exporter sequence failed. Error: %s", ex.getMessage().c_str());	
				dbError = true;
				con->terminateStatement(stmt);
				return 0;// If a failure occurs, return 0			
			}
			msg(MSG_INFO,"IpfixDbWriterOracle: new exporter (ODID=%d, id=%d) inserted in exporter table", sourceID->observationDomainId, exporterID);		
		}
	}


	if (curExporterEntries==MAX_EXP_TABLE-1) {
		// maybe here we should check how often this happens and display a severe warning if too
		// many parallel streams are received at once
		msg(MSG_INFO, "IpfixDbWriterPg: turnover for exporter cache occurred.");
		curExporterEntries = 0;
	}

	/**Write new exporter in the exporterBuffer*/
	exporterEntries[curExporterEntries].Id = exporterID;
	exporterEntries[curExporterEntries].observationDomainId = sourceID->observationDomainId;
	exporterEntries[curExporterEntries++].ip = expIp; 

	return exporterID;
}

/***** Public Methods ****************************************************/



IpfixDbWriterOracle::IpfixDbWriterOracle(const char* dbType, const char* host, const char* db,
                const char* user, const char* pw,
                unsigned int port, uint16_t observationDomainId,
                int maxStatements, vector<string> columns, bool legacyNames, const char* prefix)
        : IpfixDbWriterSQL(dbType, host, db, user, pw, port, observationDomainId, maxStatements, columns, legacyNames, prefix), con(0), env(0)
{
        connectToDB();
}

/**
 * Destructor
 */
IpfixDbWriterOracle::~IpfixDbWriterOracle()
{
	writeToDb();
	env->terminateConnection(con);
	oracle::occi::Environment::terminateEnvironment(env);
}

#endif /* ORACLE_SUPPORT_ENABLED */
