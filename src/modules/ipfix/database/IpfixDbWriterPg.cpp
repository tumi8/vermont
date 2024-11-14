/*
 * IPFIX Database Reader/Writer
 * Copyright (C) 2006 Jürgen Abberger
 * Copyright (C) 2006 Lothar Braun <braunl@informatik.uni-tuebingen.de>
 * Copyright (C) 2007 Gerhard Muenz
 * Copyright (C) 2008 Tobias Limmer
 * Copyright (C) 2014 Oliver Gasser
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
#include <boost/format.hpp>

using namespace std;

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
    DPRINTF_INFO("using connection string '%s'", conninfo.str().c_str());
    conn = PQconnectdb(conninfo.str().c_str());
    if (PQstatus(conn) != CONNECTION_OK) {
    	msg(LOG_CRIT, "IpfixDbWriterPg: Connection to database failed, error: %s", PQerrorMessage(conn));
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
	DPRINTF_INFO("PQntuples: %d", PQntuples(res));
	if((PQresultStatus(res) != PGRES_TUPLES_OK) || (PQntuples(res)==0)) {
		msg(LOG_CRIT, "IpfixDbWriterPg: Failed to check if table 'exporter' exists. Error: %s",
				PQerrorMessage(conn));
		PQclear(res);
		dbError = true;
		return 1;
	}
	if (atoi(PQgetvalue(res, 0, 0))!=1) {
		string ctexporter = "CREATE TABLE exporter (id serial PRIMARY KEY, "
																 "sourceID integer, "
																 "srcIP inet)";
		res = PQexec(conn, ctexporter.c_str());
		if(PQresultStatus(res) != PGRES_COMMAND_OK) {
			msg(LOG_CRIT, "IpfixDbWriterPg: Creation of table Exporter failed.  Error: %s",
					PQerrorMessage(conn));
			PQclear(res);
			dbError = true;
			return 1;
		} else {
			PQclear(res);
			msg(LOG_INFO, "Exporter table created");
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
		DPRINTF_INFO("Partition '%s' already created.", partitionname);
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
			msg(LOG_CRIT,"IpfixDbWriterPg: Creation of table failed. Error: %s",
					PQerrorMessage(conn));
			dbError = true;
			PQclear(res);
			return false;
		} else {
			PQclear(res);
			msg(LOG_NOTICE, "Table %s created ", tablePrefix.c_str());
		}
	}
	if (!checkRelationExists(partitionname)) {
		// create partition
		ostringstream cpsql;

		cpsql << "CREATE TABLE " << partitionname;
		cpsql << " () INHERITS (" << tablePrefix << ")";

		PGresult* res = PQexec(conn, cpsql.str().c_str());
		if (PQresultStatus(res) != PGRES_COMMAND_OK) {
			msg(LOG_CRIT,"IpfixDbWriterPg: Creation of partition failed. Error: %s",
					PQerrorMessage(conn));
			dbError = true;
			PQclear(res);
			return false;
		} else {
			PQclear(res);
			msg(LOG_NOTICE, "Partition %s created ", partitionname);
			usedPartitions.push_back(partitionname);
			if (usedPartitions.size()>MAX_USEDTABLES) usedPartitions.pop_front();
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

	DPRINTF_INFO("SQL Query: %s", insertBuffer.sql);

	// Write rows to database
	PGresult* res = PQexec(conn, insertBuffer.sql);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		msg(LOG_ERR,"IpfixDbWriterPg: Insert of records failed. Error: %s",
				PQerrorMessage(conn));
		PQclear(res);
		goto dbwriteerror;
	}
	PQclear(res);

	insertBuffer.curRows = 0;
	insertBuffer.appendPtr = insertBuffer.bodyPtr;
	*insertBuffer.appendPtr = 0;

    msg(LOG_INFO,"Write to database is complete");
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
			DPRINTF_INFO("Exporter sourceID/IP with ID %d is in the exporterBuffer\n",
					exporterEntries[i].Id);
			return exporterEntries[i].Id;
		}
	}


	// it is not: try to get it from the database
	sprintf(statementStr, "SELECT id FROM exporter WHERE sourceID=%u AND srcIp='%s'", sourceID->observationDomainId, IPToString(expIp).c_str());

	PGresult* res = PQexec(conn, statementStr);
	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		msg(LOG_ERR,"IpfixDbWriterPg: Select on exporter table failed. Error: %s",
				PQerrorMessage(conn));
		dbError = true;
		return 0;// If a failure occurs, return exporterID = 0
	}

	/** is the exporterID in the exporter table ?*/
	if (PQntuples(res)) {
		exporterID = atoi(PQgetvalue(res, 0, 0));
		DPRINTF_INFO("ExporterID %d is in exporter table\n",exporterID);

	}
	else
	{
		PQclear(res);

		/**ExporterID is not in exporter table - insert expID and expIp and return the exporterID*/
		sprintf(statementStr, "INSERT INTO exporter (sourceID, srcIP) VALUES ('%u','%s') RETURNING id",
				sourceID->observationDomainId, IPToString(expIp).c_str());

		res = PQexec(conn, statementStr);
		if(PQresultStatus(res) != PGRES_TUPLES_OK) {
			msg(LOG_ERR,"IpfixDbWriterPg: Insert in exporter table failed. Error: %s",
					PQerrorMessage(conn));
			dbError = true;
			return 0;
		}

		exporterID = atoi(PQgetvalue(res, 0, 0));
		msg(LOG_NOTICE,"ExporterID %d inserted in exporter table", exporterID);
	}
	PQclear(res);

	if (curExporterEntries==MAX_EXP_TABLE-1) {
		// maybe here we should check how often this happens and display a severe warning if too
		// many parallel streams are received at once
		msg(LOG_NOTICE, "IpfixDbWriterPg: turnover for exporter cache occurred.");
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
	        msg(LOG_CRIT, "IpfixDbWriterPg: Failed to check if relation '%s' exists. Error: %s",
                        relname, PQerrorMessage(conn));
	        PQclear(res);
		dbError = true;
		return false;
	}
	
	if (atoi(PQgetvalue(res, 0, 0))==1) {
		PQclear(res);
		return true;
	}
	PQclear(res);
	return false;
}

/**
 * In Postgres IPv4 addresses are stored as inet types and thus converted to dotted decimal notation.
 */
void IpfixDbWriterPg::parseIpfixIpv4Address(IpfixRecord::Data* data, string* parsedData) {
    *parsedData = boost::str(boost::format("'%u.%u.%u.%u'") % (int) data[0] % (int) data[1] % (int) data[2] % (int) data[3]);
}

/**
 * In Postgres IPv6 addresses are stored as inet types and thus converted to double colon hex notation.
 */
void IpfixDbWriterPg::parseIpfixIpv6Address(IpfixRecord::Data* data, string* parsedData) {
	*parsedData = boost::str(boost::format("'%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x'") % htons((uint16_t) data[0]) % htons((uint16_t) data[2]) % htons((uint16_t) data[4]) % htons((uint16_t) data[6]) % htons((uint16_t) data[8]) % htons((uint16_t) data[10]) % htons((uint16_t) data[12]) % htons((uint16_t) data[14]));
}

/**
 * In Postgres MAC addresses are stored as macaddr types and thus converted to colon hex notation.
 */
void IpfixDbWriterPg::parseIpfixMacAddress(IpfixRecord::Data* data, string* parsedData) {
    *parsedData = boost::str(boost::format("'%02x:%02x:%02x:%02x:%02x:%02x'") % (int) data[0] % (int) data[1] % (int) data[2] % (int) data[3] % (int) data[4] % (int) data[5]);
}

/***** Exported Functions ****************************************************/

IpfixDbWriterPg::IpfixDbWriterPg(const char* dbType, const char* host, const char* db,
		const char* user, const char* pw,
		unsigned int port, uint16_t observationDomainId,
		int maxStatements, vector<string> columns, bool legacyNames, const char* prefix)
	: IpfixDbWriterSQL(dbType, host, db, user, pw, port, observationDomainId, maxStatements, columns, legacyNames, prefix), conn(0)
{
	connectToDB();
}

IpfixDbWriterPg::~IpfixDbWriterPg()
{
	writeToDb();
	if (conn) PQfinish(conn);
}


#endif
