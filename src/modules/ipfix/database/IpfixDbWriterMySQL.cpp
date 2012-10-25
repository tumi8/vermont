/*
 * IPFIX Database Reader/Writer
 * Copyright (C) 2006 Jürgen Abberger
 * Copyright (C) 2006 Lothar Braun <braunl@informatik.uni-tuebingen.de>
 * Copyright (C) 2007, 2008 Gerhard Muenz
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

/* Some constants that are common to IpfixDbWriterMySQL and IpfixDbReader */
#ifdef DB_SUPPORT_ENABLED

#include <stdexcept>
#include <string.h>
#include <iomanip>
#include <stdlib.h>
#include "IpfixDbWriterMySQL.hpp"
#include "common/msg.h"

/**
 *	struct to identify column names with IPFIX_TYPEID an the dataType to store in database
 *	ExporterID is no IPFIX_TYPEID, its user specified
 *      Attention: order of entries is important!
 */
const static IpfixDbWriterMySQL::Column identifyMySQL [] = {
	{	CN_dstIP, 		IPFIX_TYPEID_destinationIPv4Address,	"INTEGER(10) UNSIGNED", 	0, 0},
	{	CN_srcIP, 		IPFIX_TYPEID_sourceIPv4Address,		"INTEGER(10) UNSIGNED", 	0, 0},
	{	CN_srcPort, 		IPFIX_TYPEID_sourceTransportPort,	"SMALLINT(5) UNSIGNED", 	0, 0},
	{	CN_dstPort, 		IPFIX_TYPEID_destinationTransportPort,	"SMALLINT(5) UNSIGNED", 	0, 0},
	{	CN_proto, 		IPFIX_TYPEID_protocolIdentifier,	"TINYINT(3) UNSIGNED", 		0, 0 },
	{	CN_dstTos, 		IPFIX_TYPEID_classOfServiceIPv4,	"TINYINT(3) UNSIGNED", 		0, 0},
	{	CN_bytes, 		IPFIX_TYPEID_octetDeltaCount,		"BIGINT(20) UNSIGNED", 		0, 0},
	{	CN_pkts, 		IPFIX_TYPEID_packetDeltaCount,		"BIGINT(20) UNSIGNED", 		0, 0},
	{	CN_firstSwitched, 	IPFIX_TYPEID_flowStartSeconds,		"INTEGER(10) UNSIGNED", 	0, 0}, // default value is invalid/not used for this ent
	{	CN_lastSwitched, 	IPFIX_TYPEID_flowEndSeconds,		"INTEGER(10) UNSIGNED", 	0, 0}, // default value is invalid/not used for this entry
	{	CN_firstSwitchedMillis, IPFIX_TYPEID_flowStartMilliSeconds,	"SMALLINT(5) UNSIGNED", 	0, 0},
	{	CN_lastSwitchedMillis,	IPFIX_TYPEID_flowEndMilliSeconds,	"SMALLINT(5) UNSIGNED", 	0, 0},
	{	CN_tcpControlBits,  	IPFIX_TYPEID_tcpControlBits,		"SMALLINT(5) UNSIGNED", 	0, 0},
	//TODO: use enterprise number for the following extended types (Gerhard, 12/2009)
	{	CN_revbytes, 		IPFIX_TYPEID_octetDeltaCount,		"BIGINT(20) UNSIGNED",		IPFIX_PEN_reverse, 0},
	{	CN_revpkts, 		IPFIX_TYPEID_packetDeltaCount,		"BIGINT(20) UNSIGNED", 		IPFIX_PEN_reverse, 0},
	{	CN_revFirstSwitched, 	IPFIX_TYPEID_flowStartSeconds,		"INTEGER(10) UNSIGNED", 	IPFIX_PEN_reverse, 0}, // default value is invalid/not used for this entry
	{	CN_revLastSwitched, 	IPFIX_TYPEID_flowEndSeconds,		"INTEGER(10) UNSIGNED", 	IPFIX_PEN_reverse, 0}, // default value is invalid/not used for this entry
	{	CN_revFirstSwitchedMillis,IPFIX_TYPEID_flowStartMilliSeconds,	"SMALLINT(5) UNSIGNED", 	IPFIX_PEN_reverse, 0},
	{	CN_revLastSwitchedMillis, IPFIX_TYPEID_flowEndMilliSeconds,	"SMALLINT(5) UNSIGNED", 	IPFIX_PEN_reverse, 0},
	{	CN_revTcpControlBits,  IPFIX_TYPEID_tcpControlBits,		"SMALLINT(5) UNSIGNED", 	IPFIX_PEN_reverse, 0},
	{	CN_maxPacketGap,	IPFIX_ETYPEID_maxPacketGap,		"BIGINT(20) UNSIGNED", 		IPFIX_PEN_vermont|IPFIX_PEN_reverse},
	{	CN_exporterID, 		EXPORTERID,				"SMALLINT(5) UNSIGNED", 	0, 0},
	{	CN_flowStartSysUpTime,	IPFIX_TYPEID_flowStartSysUpTime,	"INTEGER(10) UNSIGNED",		0, 0},
	{	CN_flowEndSysUpTime,	IPFIX_TYPEID_flowEndSysUpTime,		"INTEGER(10) UNSIGNED",		0, 0},
	{	0	} // last entry must be 0
};



/**
 * (re)connect to database
 */
void IpfixDbWriterMySQL::connectToDB()
{
	ostringstream statement;

	dbError = true;

	// close (in the case that it was already connected)
	if (conn) mysql_close(conn);

	/** get the mysl init handle*/
	conn = mysql_init(0);
	if(conn == 0) {
		msg(MSG_FATAL,"IpfixDbWriterMySQL: Get MySQL connect handle failed. Error: %s",
				mysql_error(conn));
		return;
	}
	msg(MSG_DEBUG,"IpfixDbWriterMySQL: mysql init successful");

	/**Connect to Database*/
	if (!mysql_real_connect(conn, hostName, userName, password,
				0, portNum, 0, 0)) {
		msg(MSG_FATAL,"IpfixDbWriterMySQL: Connection to database failed. Error: %s",
				mysql_error(conn));
		return;
	}
	msg(MSG_DEBUG,"IpfixDbWriterMySQL: succesfully connected to database");

	/** make query string to create database**/
	statement << "CREATE DATABASE IF NOT EXISTS " << dbName;
	DPRINTF("SQL Query: %s", statement.str().c_str());

	/**create database*/
	if(mysql_query(conn, statement.str().c_str()) != 0 ) {
		msg(MSG_FATAL, "IpfixDbWriterMySQL: Creation of database %s failed. Error: %s",
				dbName, mysql_error(conn));
		return;
	}
	msg(MSG_INFO,"IpfixDbWriterMySQL: Database %s created", dbName);

	/** use database with dbName**/
	if(mysql_select_db(conn, dbName) !=0) {
		msg(MSG_FATAL, "IpfixDbWriterMySQL: Database %s not selectable. Error: %s",
				dbName, mysql_error(conn));
		return ;
	}
	msg(MSG_DEBUG,"IpfixDbWriterMySQL: Database %s selected", dbName);
	if (createExporterTable() != 0) return;

	dbError = false;
	
}


int IpfixDbWriterMySQL::createExporterTable()
{
	ostringstream statement;

	dbError = true;

	/**create table exporter*/
	statement.str("");
	statement.clear();
	statement << "CREATE TABLE IF NOT EXISTS exporter (id SMALLINT(5) NOT NULL AUTO_INCREMENT, sourceID INTEGER(10) UNSIGNED DEFAULT NULL, srcIP INTEGER(10) UNSIGNED DEFAULT NULL, PRIMARY KEY(id))";
	DPRINTF("SQL Query: %s", statement.str().c_str());
	if(mysql_query(conn, statement.str().c_str()) != 0) {
		msg(MSG_FATAL,"IpfixDbWriterMySQL: Creation of exporter table failed. Error: %s",
				mysql_error(conn));
		return 1;
	}
	msg(MSG_INFO,"IpfixDbWriterMySQL: Exporter table created");

	dbError = false;

	return 0;
}

/**
 * 	Create a table in the database
 */
bool IpfixDbWriterMySQL::createDBTable(const char* partitionname, uint64_t starttime, uint64_t endtime)
{
	uint32_t i;

	if (find(usedPartitions.begin(), usedPartitions.end(), partitionname)!=usedPartitions.end()) {
		// found cached entry!
		DPRINTF("Partition '%s' already created.", partitionname);
		return true;
	}

	ostringstream ctsql;

	ostringstream oss;
	ctsql << "CREATE TABLE IF NOT EXISTS " << partitionname << " (";
	/**collect the names for columns and the dataTypes for the table in a string*/
	for(i=0; i < numberOfColumns; i++) {
		ctsql << identify[i].cname << " " << identify[i].dataType;
		if (i != numberOfColumns-1) {
			ctsql << ", ";
		}
	}
	ctsql << ")";


	if(mysql_query(conn, ctsql.str().c_str()) != 0) {
		msg(MSG_FATAL,"IpfixDbWriterMySQL: Creation of exporter table failed. Error: %s",
				mysql_error(conn));
		dbError = true;
		return 1;
	}

	msg(MSG_INFO, "Partition %s created ", partitionname);
	usedPartitions.push_back(partitionname);
	if (usedPartitions.size()>MAX_USEDTABLES) usedPartitions.pop_front();

	string indexname = string(partitionname) + "_firstswitched";
	ostringstream cisql;
	cisql << "CREATE INDEX " << indexname <<" ON " << partitionname;
	cisql << "(firstswitched)";
	if(mysql_query(conn, ctsql.str().c_str()) != 0) {
		msg(MSG_FATAL,"IpfixDbWriterMySQL: Creation of index failed. Error: %s",
			mysql_error(conn));
		dbError = true;
		return true;
	}
	msg(MSG_INFO, "Index %s_firstswitched created ", partitionname);

	return true;
}


/**
 *	loop over table columns and template to get the IPFIX values in correct order to store in database
 *	The result is written into row, the firstSwitched time is returned in flowstartsec
 */

/*
 * Write insertStatement to database
 */
bool IpfixDbWriterMySQL::writeToDb()
{
	if (insertBuffer.curRows == 0) return true;

	// delete last comma from sql string, as it is always inserted by fillRowColumns
	insertBuffer.appendPtr[-1] = 0;

	DPRINTF("SQL Query: %s", insertBuffer.sql);

	if(mysql_query(conn, insertBuffer.sql) != 0) {
		msg(MSG_ERROR,"IpfixDbWriterMySQL: Insert of records failed. Error: %s", mysql_error(conn));
		goto dbwriteerror;
	}

	insertBuffer.curRows = 0;
	insertBuffer.appendPtr = insertBuffer.bodyPtr;
	*insertBuffer.appendPtr = 0;

	msg(MSG_DEBUG,"IpfixDbWriterMySQL: Write to database is complete");
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
int IpfixDbWriterMySQL::getExporterID(IpfixRecord::SourceID* sourceID)
{
	uint32_t i;
	int exporterID = 0;
	MYSQL_RES* dbResult;
	MYSQL_ROW dbRow;

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

	if(mysql_query(conn, statementStr) != 0) {
		msg(MSG_ERROR,"IpfixDbWriterMySQL: Select on exporter table failed. Error: %s",
				mysql_error(conn));
		return 0;// If a failure occurs, return 0
	}

	dbResult = mysql_store_result(conn);
	if(( dbRow = mysql_fetch_row(dbResult))) {
		// found in table
		exporterID = atoi(dbRow[0]);
		mysql_free_result(dbResult);
		DPRINTF("ExporterID %d is in exporter table", id);
	} else {
		mysql_free_result(dbResult);
		// insert new exporter table entry

		/**ExporterID is not in exporter table - insert expID and expIp and return the exporterID*/
		sprintf(statementStr, "INSERT INTO exporter (sourceID, srcIP) VALUES ('%u','%s')",
				sourceID->observationDomainId, IPToString(expIp).c_str());

		if(mysql_query(conn, statementStr) != 0) {
			msg(MSG_ERROR,"IpfixDbWriterMySQL: Insert in exporter table failed. Error: %s", conn);
			return 0;
		}

		exporterID = mysql_insert_id(conn);
		msg(MSG_INFO,"IpfixDbWriterMySQL: new exporter (ODID=%d, id=%d) inserted in exporter table", sourceID->observationDomainId, exporterID);
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

IpfixDbWriterSQL::Column* IpfixDbWriterMySQL::fillColumnStructure()
{
	return (Column*)identifyMySQL;
}


/***** Public Methods ****************************************************/

IpfixDbWriterMySQL::IpfixDbWriterMySQL(const char* host, const char* db,
		const char* user, const char* pw,
		unsigned int port, uint16_t observationDomainId,
		int maxStatements)
	: IpfixDbWriterSQL(host, db, user, pw, port, observationDomainId, maxStatements)
{
	identify = fillColumnStructure();

	/**count columns*/
	numberOfColumns = 0;
	for(uint32_t i=0; identify[i].cname!=0; i++) numberOfColumns++;

	/**Initialize structure members Statement*/
	insertBuffer.curRows = 0;
	insertBuffer.maxRows = maxStatements;
	insertBuffer.sql = new char[(INS_WIDTH+3)*(numberOfColumns+1)*maxStatements+numberOfColumns*20+60+1];
	*insertBuffer.sql = 0;

	connectToDB();
}

IpfixDbWriterMySQL::~IpfixDbWriterMySQL()
{
	writeToDb();
	if (conn) mysql_close(conn);
}



#endif
