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

/* Some constants that are common to IpfixDbWriterPg and IpfixDbReader */
#ifdef PG_SUPPORT_ENABLED

#include "IpfixDbWriterPg.hpp"
#include "common/msg.h"
#include "common/Misc.h"
#include "common/Time.h"

#include <stdexcept>
#include <string.h>
#include <stdlib.h>
#include <sstream>
#include <algorithm>

using namespace std;


/***** Global Variables ******************************************************/

/**
 * maximum length of one item in a SQL statement
 */
const uint16_t MAX_COL_LENGTH = 22;



/**
 * (re)connect to database
 */
void IpfixDbWriterPg::connectToDB()
{
    dbError = true;

    // close (in the case that it was already connected)
    if (conn) PQfinish(conn);


    /**Connect to Database*/
    ostringstream conninfo;
    conninfo << "host='" << hostName << "' port='" << portNum << "' ";
    conninfo << "dbname='" << dbName << "' user='" << userName<< "' ";
    conninfo << "password='" << password << "'";// sslmode=require";
    DPRINTF("using connection string '%s'", conninfo.str().c_str());
    conn = PQconnectdb(conninfo.str().c_str());
    if (PQstatus(conn) != CONNECTION_OK) {
    	msg(MSG_FATAL, "IpfixDbWriterPg: Connection to database failed, error: %s", PQerrorMessage(conn));
    	return;
    }
    /**create table exporter*/
    if (createExporterTable()!=0) return;

    dbError = false;
}



int IpfixDbWriterPg::createExporterTable()
{
	/** create table exporter*/
	ostringstream oss;
	oss << "SELECT COUNT(*) FROM pg_class where relname='exporter'";
	PGresult* res = PQexec(conn, oss.str().c_str());
	DPRINTF("PQntuples: %d", PQntuples(res));
	if((PQresultStatus(res) != PGRES_TUPLES_OK) || (PQntuples(res)==0)) {
		msg(MSG_FATAL, "IpfixDbWriterPg: Failed to check if table 'exporter' exists. Error: %s",
				PQerrorMessage(conn));
		PQclear(res);
		return 1;
	}
	if (atoi(PQgetvalue(res, 0, 0))!=1) {
		string ctexporter = "CREATE TABLE exporter (id serial PRIMARY KEY, "
																 "sourceID integer, "
																 "srcIP inet)";
		res = PQexec(conn, ctexporter.c_str());
		if(PQresultStatus(res) != PGRES_COMMAND_OK) {
			msg(MSG_FATAL, "IpfixDbWriterPg: Creation of table Exporter failed.  Error: %s",
					PQerrorMessage(conn));
			PQclear(res);
			return 1;
		} else {
			PQclear(res);
			msg(MSG_DEBUG, "Exporter table created");
		}
	}

	return 0;
}

/**
 * 	Create a table in the database
 */
bool IpfixDbWriterPg::createDBTable(const char* partitionname, uint64_t starttime, uint64_t endtime)
{
	uint32_t i;

	if (find(usedPartitions.begin(), usedPartitions.end(), partitionname)!=usedPartitions.end()) {
		// found cached entry!
		DPRINTF("Partition '%s' already created.", partitionname);
		return true;
	}

	ostringstream ctsql;

	ostringstream oss;
	// check if table needs to be created
	if (!checkRelationExists(tablePrefix.c_str())) {
		ctsql << "CREATE TABLE " << tablePrefix << " (";
		/**collect the names for columns and the dataTypes for the table in a string*/
		for(i=0; i < numberOfColumns; i++) {
			ctsql << tableColumns[i].cname << " " << tableColumns[i].dataType;
			if (i != numberOfColumns-1) {
				ctsql << ", ";
			}
		}
		ctsql << ")";

		/** create table*/
		PGresult* res = PQexec(conn, ctsql.str().c_str());
		if (PQresultStatus(res) != PGRES_COMMAND_OK) {
			msg(MSG_FATAL,"IpfixDbWriterPg: Creation of table failed. Error: %s",
					PQerrorMessage(conn));
			dbError = true;
			PQclear(res);
			return false;
		} else {
			PQclear(res);
			msg(MSG_INFO, "Table %s created ", tablePrefix.c_str());
		}
	}
	if (!checkRelationExists(partitionname)) {
		// create partition
		ostringstream cpsql;

		cpsql << "CREATE TABLE " << partitionname << " (CHECK (firstswitched>='";
		//cpsql << getTimeAsString(starttime, "%Y-%m-%d %H:%M:%S", true);
		cpsql << starttime;
		cpsql << "' AND firstswitched<'" << endtime; //getTimeAsString(endtime, "%Y-%m-%d %H:%M:%S", true);
		cpsql << "')) INHERITS (" << tablePrefix << ")";

		PGresult* res = PQexec(conn, cpsql.str().c_str());
		if (PQresultStatus(res) != PGRES_COMMAND_OK) {
			msg(MSG_FATAL,"IpfixDbWriterPg: Creation of partition failed. Error: %s",
					PQerrorMessage(conn));
			dbError = true;
			PQclear(res);
			return false;
		} else {
			PQclear(res);
			msg(MSG_INFO, "Partition %s created ", partitionname);
			usedPartitions.push_back(partitionname);
			if (usedPartitions.size()>MAX_USEDTABLES) usedPartitions.pop_front();
		}
	}
	string indexname = string(partitionname) + "_firstswitched";
	if (!checkRelationExists(indexname.c_str())) {
		ostringstream cisql;
		cisql << "CREATE INDEX " << indexname <<" ON " << partitionname;
		cisql << "(firstswitched)";
		PGresult* res = PQexec(conn, cisql.str().c_str());
		if (PQresultStatus(res) != PGRES_COMMAND_OK) {
			msg(MSG_FATAL,"IpfixDbWriterPg: Creation of index failed. Error: %s",
					PQerrorMessage(conn));
			dbError = true;
			PQclear(res);
			return false;
		} else {
			PQclear(res);
			msg(MSG_INFO, "Index %s_firstswitched created ", partitionname);
		}
	}
	return true;
}

/**
 *	Function writes the content of the statemBuffer to database
 *	statemBuffer consist of single insert statements
 */
bool IpfixDbWriterPg::writeToDb()
{
	if (insertBuffer.curRows==0) return true;

	// delete last comma from sql string, as it is always inserted by fillRowColumns
	insertBuffer.appendPtr[-1] = 0;

	// Write rows to database
	msg(MSG_FATAL, "%s", insertBuffer.sql);
	PGresult* res = PQexec(conn, insertBuffer.sql);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		msg(MSG_ERROR,"IpfixDbWriterPg: Insert of records failed. Error: %s",
				PQerrorMessage(conn));
		PQclear(res);
		goto dbwriteerror;
	}
	PQclear(res);

	insertBuffer.curRows = 0;
	insertBuffer.appendPtr = insertBuffer.bodyPtr;
	*insertBuffer.appendPtr = 0;

    msg(MSG_DEBUG,"Write to database is complete");
    return true;

dbwriteerror:
    dbError = true;
	return false;
}

/**
 *	Returns the exporterID
 *  	For every different sourcID and expIp a unique ExporterID will be generated from the database
 * 	First lookup for the ExporterID in the exporterBuffer according sourceID and expIp, is there nothing
 *  	lookup in the ExporterTable, is there also nothing insert sourceID and expIp an return the generated
 *      ExporterID
 */
int IpfixDbWriterPg::getExporterID(IpfixRecord::SourceID* sourceID)
{
	uint32_t i;
	int exporterID = 0;

	char statementStr[EXPORTER_WIDTH];
	uint32_t expIp = 0;

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


	// it is not: try to get it from the database
	sprintf(statementStr, "SELECT id FROM exporter WHERE sourceID=%u AND srcIp='%s'", sourceID->observationDomainId, IPToString(expIp).c_str());

	PGresult* res = PQexec(conn, statementStr);
	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		msg(MSG_ERROR,"IpfixDbWriterPg: Select on exporter table failed. Error: %s",
				PQerrorMessage(conn));
		return 0;// If a failure occurs, return exporterID = 0
	}

	/** is the exporterID in the exporter table ?*/
	if (PQntuples(res)) {
		exporterID = atoi(PQgetvalue(res, 0, 0));
		DPRINTF("ExporterID %d is in exporter table\n",exporterID);

	}
	else
	{
		PQclear(res);

		/**ExporterID is not in exporter table - insert expID and expIp and return the exporterID*/
		sprintf(statementStr, "INSERT INTO exporter (sourceID, srcIP) VALUES ('%u','%s') RETURNING id",
				sourceID->observationDomainId, IPToString(expIp).c_str());

		res = PQexec(conn, statementStr);
		if(PQresultStatus(res) != PGRES_TUPLES_OK) {
			msg(MSG_ERROR,"IpfixDbWriterPg: Insert in exporter table failed. Error: %s",
					PQerrorMessage(conn));
			return 0;
		}

		exporterID = atoi(PQgetvalue(res, 0, 0));
		msg(MSG_INFO,"ExporterID %d inserted in exporter table", exporterID);
	}
	PQclear(res);

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

bool IpfixDbWriterPg::checkRelationExists(const char* relname)
{
	// check if table needs to be created
	ostringstream oss;
	oss << "SELECT COUNT(*) FROM pg_class where relname='" << relname << "'";
	PGresult* res = PQexec(conn, oss.str().c_str());
	if((PQresultStatus(res) != PGRES_TUPLES_OK) || (PQntuples(res)==0)) {
	        msg(MSG_FATAL, "IpfixDbWriterPg: Failed to check if relation '%s' exists. Error: %s",
                        relname, PQerrorMessage(conn));
	        PQclear(res);
		return false;
	}
	
	if (atoi(PQgetvalue(res, 0, 0))==1) {
		PQclear(res);
		return true;
	}
	PQclear(res);
	return false;
}

std::string IpfixDbWriterPg::getDBDataType(uint16_t ipfixTypeLength)
{
	// TODO: postgres does not do unsigned types. we therefore use the bigger field. this wastes 
	/// disk space. Optimize! (except bigints ...)
	switch (ipfixTypeLength) {
	case 1:
		return "smallint";
	case 2:
                return "integer";
        case 4:
                return "bigint";
        case 8:
                return "bigint";
	case 65535:
		// variable length, we only support fields up to 100 bytes (be careful, this may waste a lot of diskspace ...")
		return "VARCHAR(100)";
        default:
                THROWEXCEPTION("IpfixDbReaderPg: Type with non matching length %d ", ipfixTypeLength);
	}
	// make compiler happy. we should never get here
	return "";
}



/***** Exported Functions ****************************************************/

IpfixDbWriterPg::IpfixDbWriterPg(const char* dbType, const char* host, const char* db,
		const char* user, const char* pw,
		unsigned int port, uint16_t observationDomainId,
		int maxStatements, vector<string> columns, bool legacyNames)
	: IpfixDbWriterSQL(dbType, host, db, user, pw, port, observationDomainId, maxStatements, columns, legacyNames), conn(0)
{
	connectToDB();
}

IpfixDbWriterPg::~IpfixDbWriterPg()
{
	writeToDb();
	if (conn) PQfinish(conn);
}


#endif
