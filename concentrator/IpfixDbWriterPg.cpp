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

#include <stdexcept>
#include <string.h>
#include <stdlib.h>
#include <sstream>

using namespace std;


/***** Global Variables ******************************************************/

/**
 * maximum length of one item in a SQL statement
 */
const uint16_t MAX_COL_LENGTH = 22;

/**
 *	struct to identify column names with IPFIX_TYPEID an the dataType to store in database
 *	ExporterID is no IPFIX_TYPEID, its user specified
 *      Attention: order of entries is important!
 */
const uint16_t ID_FIRSTSWITCHED_IDX = 8;
const uint16_t ID_LASTSWITCHED_IDX = 9;
const IpfixDbWriterPg::Column IpfixDbWriterPg::identify [] = {
	{	"dstIP", IPFIX_TYPEID_destinationIPv4Address, "inet",0},
	{	"srcIP", IPFIX_TYPEID_sourceIPv4Address, "inet", 0},
	{	"srcPort", IPFIX_TYPEID_sourceTransportPort, "integer", 0},
	{	"dstPort", IPFIX_TYPEID_destinationTransportPort, "integer",0},
	{	"proto",IPFIX_TYPEID_protocolIdentifier , "smallint", 0},
	{	"dstTos", IPFIX_TYPEID_classOfServiceIPv4, "smallint", 0},
	{	"bytes", IPFIX_TYPEID_octetDeltaCount, "bigint", 0},
	{	"pkts", IPFIX_TYPEID_packetDeltaCount, "bigint", 0},
	{	"firstSwitched", IPFIX_TYPEID_flowStartMilliSeconds, "bigint", 0}, // default value is invalid/not used for this entry
	{	"lastSwitched", IPFIX_TYPEID_flowEndMilliSeconds, "bigint", 0}, // default value is invalid/not used for this entry
	{	"tcpControlBits", IPFIX_TYPEID_tcpControlBits,  "smallint", 0},
	{	"revbytes", IPFIX_ETYPEID_revOctetDeltaCount, "bigint", 0},
	{	"revpkts", IPFIX_ETYPEID_revPacketDeltaCount, "bigint", 0},
	{	"revFirstSwitched", IPFIX_ETYPEID_revFlowStartMilliSeconds, "bigint", 0}, // default value is invalid/not used for this entry
	{	"revLastSwitched", IPFIX_ETYPEID_revFlowEndMilliSeconds, "bigint", 0}, // default value is invalid/not used for this entry
	{	"revTcpControlBits", IPFIX_ETYPEID_revTcpControlBits,  "smallint", 0},
	{	"exporterID",EXPORTERID, "integer", 0},
	{	0} // last entry must be 0
};



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
    conninfo << "password='" << password << "' sslmode=require";
    DPRINTF("using connection string '%s'", conninfo.str().c_str());
    conn = PQconnectdb(conninfo.str().c_str());
    /*if (!mysql_real_connect(conn,
		hostName, userName,
		password, 0, portNum,
		socketName, flags)) {
		msg(MSG_FATAL,"IpfixDbWriterPg: Connection to database failed. Error: %s",
			mysql_error(conn));
		return;
    } else {
    	msg(MSG_DEBUG,"IpfixDbWriterPg succesfully connected to database");
    }*/
    if (PQstatus(conn) != CONNECTION_OK) {
    	msg(MSG_FATAL, "IpfixDbWriterPg: Connection to database failed, error: %s", PQerrorMessage(conn));
    	return;
    }
    /**create table exporter*/
    if (createExporterTable()!=0) return;
    if (createDBTable(tableName.c_str())) return;

    dbError = false;
}



int IpfixDbWriterPg::createExporterTable()
{
	/**is there already a table with the same name - drop it */
	/* gerhard: let's keep the database, we do not want to lose data
	 char dropExporterTab[STARTLEN];
	 strcpy(dropExporterTab,"DROP TABLE IF EXISTS exporter");
	 if(mysql_query(conn, dropExporterTab) != 0) {
	 msg(MSG_FATAL,"Drop of exists exporter table failed. Error: %s",
	 mysql_error(conn));
	 return 1;
	 }
	 */
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
			msg(MSG_FATAL, "IpfixDbWriterPg: Creation of table Exporter failed. Error: %s",
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
int IpfixDbWriterPg::createDBTable(const char* tablename)
{
	uint32_t i;
	
	ostringstream ctsql;
	
	ostringstream oss;
	// check if table needs to be created
	oss << "SELECT COUNT(*) FROM pg_class where relname='" << tablename << "'";
	PGresult* res = PQexec(conn, oss.str().c_str());
	if((PQresultStatus(res) != PGRES_TUPLES_OK) || (PQntuples(res)==0)) {
		msg(MSG_FATAL, "IpfixDbWriterPg: Failed to check if table 'exporter' exists. Error: %s",
				PQerrorMessage(conn));
		PQclear(res);
		return 1;
	}
	if (atoi(PQgetvalue(res, 0, 0))!=1) {
		PQclear(res);
		ctsql << "CREATE TABLE " << tablename << " (";
		/**collect the names for columns and the dataTypes for the table in a string*/
		for(i=0; i < numberOfColumns; i++) {
			ctsql << identify[i].cname << " " << identify[i].dataType;
			if (i != numberOfColumns-1) {
				ctsql << ", ";
			}
		}
		ctsql << ")";
	
		/** create table*/
		res = PQexec(conn, ctsql.str().c_str());
		if (PQresultStatus(res) != PGRES_COMMAND_OK) {
			msg(MSG_FATAL,"IpfixDbWriterPg: Creation of table failed. Error: %s",
					PQerrorMessage(conn));
			dbError = true;
			PQclear(res);
			return 1;
		}
		else {
			PQclear(res);
			msg(MSG_INFO, "Table %s created ",tablename);
		}
	}
	return 0;
}

/**
 *	function receive the DataRecord or DataDataRecord when callback is started
 */
void IpfixDbWriterPg::onDataDataRecord(IpfixDataDataRecord* record)
{
	processDataDataRecord(record->sourceID.get(), record->dataTemplateInfo.get(), 
			record->dataLength, record->data);
	
	record->removeReference();
}

/**
 * save given elements of record to database
 */
void IpfixDbWriterPg::processDataDataRecord(IpfixRecord::SourceID* sourceID, 
		IpfixRecord::DataTemplateInfo* dataTemplateInfo, uint16_t length, 
		IpfixRecord::Data* data)
{
	DPRINTF("Processing data record");

	if (dbError) {
		connectToDB();
		if (dbError) return;
	}

	/** check if statement buffer is not full*/
	if(insertBuffer.curRows==insertBuffer.maxRows) {
		THROWEXCEPTION("IpfixDbWriterPg: Statement buffer is full, this should never happen! Aborting ...");
	}

	/** sourceid null ? use default*/
	/* overwrite sourceid if defined */
	if(srcId.observationDomainId != 0 || sourceID == NULL) {
		sourceID = &srcId;
	}

	// insert record into buffer
	fillInsertRow(sourceID, dataTemplateInfo, length, data);
	
	// statemBuffer is filled ->  insert in table
	if(insertBuffer.curRows==insertBuffer.maxRows) {
		msg(MSG_INFO, "Writing buffered records to database");
		writeToDb();
	}
}


/**
 *	function receive the  when callback is started
 */
void IpfixDbWriterPg::onDataRecord(IpfixDataRecord* record)
{
	// convert templateInfo to dataTemplateInfo
	IpfixRecord::DataTemplateInfo dataTemplateInfo;
	dataTemplateInfo.templateId = 0;
	dataTemplateInfo.preceding = 0;
	dataTemplateInfo.freePointers = false; // don't free the given pointers, as they are taken from a different structure
	dataTemplateInfo.fieldCount = record->templateInfo->fieldCount; /**< number of regular fields */
	dataTemplateInfo.fieldInfo = record->templateInfo->fieldInfo; /**< array of FieldInfos describing each of these fields */
	dataTemplateInfo.dataCount = 0; /**< number of fixed-value fields */
	dataTemplateInfo.dataInfo = NULL; /**< array of FieldInfos describing each of these fields */
	dataTemplateInfo.data = NULL; /**< data start pointer for fixed-value fields */
	dataTemplateInfo.userData = record->templateInfo->userData; /**< pointer to a field that can be used by higher-level modules */

	processDataDataRecord(record->sourceID.get(), &dataTemplateInfo, record->dataLength, record->data);
	
	record->removeReference();
}


/**
 *	loop over the IpfixRecord::DataTemplateInfo (fieldinfo,datainfo) to get the IPFIX values to store in database
 *  results are stored in insertBuffer.sql
 */
void IpfixDbWriterPg::fillInsertRow(IpfixRecord::SourceID* sourceID,
		IpfixRecord::DataTemplateInfo* dataTemplateInfo, uint16_t length, IpfixRecord::Data* data)
{
	uint32_t j, k;
	uint64_t intdata = 0;
	
	strcat(insertBuffer.appendPtr++, "(");

	/**loop over the columname and loop over the IPFIX_TYPEID of the record
	 to get the corresponding data to store */
	for( j=0; j<numberOfColumns; j++) {
		bool notfound = true;

		if (identify[j].ipfixId == EXPORTERID) {
			/**lookup exporter buffer to get exporterID from sourcID and expIp**/
			uint32_t expID = getExporterID(sourceID);
			intdata = expID;
			notfound = false;
		} else {
			// try to gather data required for the field
			if(dataTemplateInfo->fieldCount > 0) {
				// look inside the ipfix data packet
				for(k=0; k < dataTemplateInfo->fieldCount; k++) {
					if(dataTemplateInfo->fieldInfo[k].type.id == identify[j].ipfixId) {
						notfound = false;
						intdata = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset));
						DPRINTF("IpfixDbWriterPg::getRecData: really saw ipfix id %d in packet with intdata %llX, type %d, length %d and offset %X", identify[j].ipfixId, intdata, dataTemplateInfo->fieldInfo[k].type.id, dataTemplateInfo->fieldInfo[k].type.length, dataTemplateInfo->fieldInfo[k].offset);
					}
				}
			}
			if( dataTemplateInfo->dataCount > 0 && notfound) {
				// look in static data fields of template for data
				for(k=0; k < dataTemplateInfo->dataCount; k++) {
					if(dataTemplateInfo->dataInfo[k].type.id == identify[j].ipfixId) {
						notfound = false;
						intdata = getdata(dataTemplateInfo->dataInfo[k].type,(dataTemplateInfo->data+dataTemplateInfo->dataInfo[k].offset));
					}
				}
			}
			if(notfound) {
				// for some Ids, we have an alternative
				switch (identify[j].ipfixId) {
					case IPFIX_TYPEID_flowStartMilliSeconds:
						// look for alternative (flowStartMilliSeconds/1000)
						if(dataTemplateInfo->fieldCount > 0) {
							for(k=0; k < dataTemplateInfo->fieldCount; k++) {
								if(dataTemplateInfo->fieldInfo[k].type.id == IPFIX_TYPEID_flowStartSeconds) {
									intdata = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset)) * 1000;
									notfound = false;
									break;
								}
							}
						}
						break;
					case IPFIX_ETYPEID_revFlowStartMilliSeconds:
						// look for alternative (revFlowStartMilliSeconds/1000)
						if(dataTemplateInfo->fieldCount > 0) {
							for(k=0; k < dataTemplateInfo->fieldCount; k++) {
								if(dataTemplateInfo->fieldInfo[k].type.id == IPFIX_ETYPEID_revFlowStartSeconds) {
									intdata = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset)) * 1000;
									notfound = false;
									break;
								}
							}
						}
						break;
					case IPFIX_TYPEID_flowEndMilliSeconds:
						// look for alternative (flowEndMilliSeconds/1000)
						if(dataTemplateInfo->fieldCount > 0) {
							for(k=0; k < dataTemplateInfo->fieldCount; k++) {
								if(dataTemplateInfo->fieldInfo[k].type.id == IPFIX_TYPEID_flowEndSeconds) {
									intdata = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset)) * 1000;
									notfound = false;
									break;
								}
							}
						}
						break;
					case IPFIX_ETYPEID_revFlowEndMilliSeconds:
						// look for alternative (revFlowEndMilliSeconds/1000)
						if(dataTemplateInfo->fieldCount > 0) {
							for(k=0; k < dataTemplateInfo->fieldCount; k++) {
								if(dataTemplateInfo->fieldInfo[k].type.id == IPFIX_ETYPEID_revFlowEndSeconds) {
									intdata = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset)) * 1000;
									notfound = false;
									break;
								}
							}
						}
						break;
				}
				// if still not found, get default value
				if(notfound)
					intdata = getdefaultIPFIXdata(identify[j].ipfixId);
			}

			// we need extra treatment for timing related fields
			switch (identify[j].ipfixId) {
				case IPFIX_TYPEID_flowStartMilliSeconds:
					if(intdata==0) {
						// if no flow start time is available, maybe this is is from a netflow from Cisco
						// then use flowStartSysUpTime as flow start time
						for(k=0; k < dataTemplateInfo->fieldCount; k++) {
							if(dataTemplateInfo->fieldInfo[k].type.id == IPFIX_TYPEID_flowStartSysUpTime) {
								intdata = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset))*1000;
							}
						}
					}
					break;
					
				case IPFIX_TYPEID_flowEndMilliSeconds:
					if(intdata==0) {
						// if no flow end time is available, maybe this is is from a netflow from Cisco
						// then use flowEndSysUpTime as flow start time
						for(k=0; k < dataTemplateInfo->fieldCount; k++) {
							if(dataTemplateInfo->fieldInfo[k].type.id == IPFIX_TYPEID_flowEndSysUpTime) {
								intdata = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset))*1000;
							}
						}
					}
					break;
			}
		}
	
		
		DPRINTF("saw ipfix id %d in packet with intdata %llX", identify[j].ipfixId, intdata);
		
		switch (identify[j].ipfixId) {
			// convert IPv4 address to string notation, as this is required by Postgres
			case IPFIX_TYPEID_sourceIPv4Address:
			case IPFIX_TYPEID_destinationIPv4Address:
				addColumnEntry(IPToString(intdata).c_str(), true, j==numberOfColumns-1);
				break;
			// all other integer data is directly converted to a string
			default:
				addColumnEntry(intdata, false, j==numberOfColumns-1);
				break;
		}
	}
	
	insertBuffer.curRows++;
	strcat(insertBuffer.appendPtr, "),");
	insertBuffer.appendPtr += 2;
}

/**
 * adds an entry for an sql statement
 */
void IpfixDbWriterPg::addColumnEntry(const char* insert, bool quoted, bool lastcolumn)
{
	if (quoted) strcat(insertBuffer.appendPtr++, "'");
	strncat(insertBuffer.appendPtr, insert, MAX_COL_LENGTH);
	insertBuffer.appendPtr += strlen(insert);
	if (quoted) strcat(insertBuffer.appendPtr++, "'");
	if (!lastcolumn) strcat(insertBuffer.appendPtr++, ",");
}

/**
 * adds an entry for an sql statement
 */
void IpfixDbWriterPg::addColumnEntry(const uint64_t insert, bool quoted, bool lastcolumn)
{
	char strdata[30];
	snprintf(strdata, ARRAY_SIZE(strdata), "%Lu", insert);
	addColumnEntry(strdata, quoted, lastcolumn);
}

/**
 *	Function writes the content of the statemBuffer to database
 *	statemBuffer consist of single insert statements
 */
int IpfixDbWriterPg::writeToDb()
{
	//uint32_t i;
	//ostringstream sql;

	/*PGresult* res = PQexec(conn, "BEGIN");
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		msg(MSG_ERROR,"IpfixDbWriterPg: Begin transaction failed. Error: %s",
				PQerrorMessage(conn));
		PQclear(res);
		goto dbwriteerror;
	}
	PQclear(res);
	
	// create prepared statement	
	sql << "INSERT INTO " << tableName << " (";
	for (uint32_t i=0; i<numberOfColumns; i++) {
		sql << identify[i].cname;
		if (i<numberOfColumns-1) sql << ", ";
	}
	sql << ") VALUES (";
	for (uint32_t i=0; i<numberOfColumns; i++) {
		sql << "$" << (i+1);
		if (i<numberOfColumns-1) sql << ", ";
	}
	sql << ")";
	res = PQprepare(conn, "", sql.str().c_str(), numberOfColumns, NULL);
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		msg(MSG_ERROR,"IpfixDbWriterPg: Preparing SQL statement failed. Error: %s",
				PQerrorMessage(conn));
		PQclear(res);
		goto dbwriteerror;
	}
	PQclear(res);*/
	
	if (insertBuffer.curRows==0) return 0;
	
	// delete last comma from sql string, as it is always inserted by fillRowColumns
	insertBuffer.appendPtr[-1] = 0;
	
	// Write rows to database
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
	
	// flush buffer to database
	/*if (PQflush(conn)!=0) {
		msg(MSG_ERROR, "IpfixDbWriterPg: SQL statement flush failed. Error: %s",
					PQerrorMessage(conn));
			goto dbwriteerror;
	}
	
	
	res = PQexec(conn, "COMMIT");
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		msg(MSG_ERROR,"IpfixDbWriterPg: Commit transaction failed. Error: %s",
				PQerrorMessage(conn));
		PQclear(res);
		goto dbwriteerror;
	}
	PQclear(res);*/
    msg(MSG_DEBUG,"Write to database is complete");
    return 0;

dbwriteerror:
    dbError = true;
	return 1;
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
	
	if(sourceID->exporterAddress.len == 4) 
		expIp = *(uint32_t*)(sourceID->exporterAddress.ip);

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

/**
 *	Get data of the record is given by the IPFIX_TYPEID
 */
uint64_t IpfixDbWriterPg::getdata(IpfixRecord::FieldInfo::Type type, IpfixRecord::Data* data)
{
	if(type.id == IPFIX_TYPEID_sourceIPv4Address || type.id == IPFIX_TYPEID_destinationIPv4Address)
	return getipv4address(type, data);
	else
	return getIPFIXValue(type, data);
}
/**
 *	determine the ipv4address of the data record
 */
uint32_t IpfixDbWriterPg::getipv4address( IpfixRecord::FieldInfo::Type type, IpfixRecord::Data* data)
{

	if (type.length > 5) {
		DPRINTF("IPv4 Address with length %d unparseable\n", type.length);
		return 0;
	}

	if ((type.length == 5) && ( type.id == IPFIX_TYPEID_sourceIPv4Address || IPFIX_TYPEID_destinationIPv4Address )) /*&& (imask != 0)*/{
		DPRINTF("imask drop from ipaddress\n");
		type.length = 4;
	}

	if ((type.length < 5) &&( type.id == IPFIX_TYPEID_sourceIPv4Address || type.id == IPFIX_TYPEID_destinationIPv4Address)) /*&& (imask == 0)*/{
		return getIPFIXValue(type, data);
	}

	return 0;
}

/**
 *	get the IPFIX value 
 */
uint64_t IpfixDbWriterPg::getIPFIXValue(IpfixRecord::FieldInfo::Type type, IpfixRecord::Data* data)
{
	switch (type.length) {
		case 1:
			return (*(uint8_t*)data);
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
uint32_t IpfixDbWriterPg::getdefaultIPFIXdata(int ipfixtype_id)
{
	int i;
	for( i=0; identify[i].cname != 0; i++) {
		if(ipfixtype_id == identify[i].ipfixId) {
			return identify[i].defaultValue;
		}
	}
	return 0;
}

/***** Exported Functions ****************************************************/

/**
 * Creates a new IpfixDbWriterPg. Do not forget to call @c startipfixDbWriter() to begin writing to Database
 * @return handle to use when calling @c destroyipfixDbWriter()
 */
IpfixDbWriterPg::IpfixDbWriterPg(const char* host, const char* db,
		const char* user, const char* pw,
		unsigned int port, uint16_t observationDomainId,
		int maxStatements)
{
	/**Initialize structure members IpfixDbWriterPg*/
	hostName = host;
	dbName = db;
	userName = user;
	password = pw;
	portNum = port;
	conn = 0;
	socketName = 0;
	flags = 0;
    srcId.exporterAddress.len = 0;
	srcId.observationDomainId = observationDomainId;
    srcId.exporterPort = 0;
    srcId.receiverPort = 0;
    srcId.protocol = 0;
    srcId.fileDescriptor = 0;
    tableName = "flows";
    bzero(&exporterEntries, sizeof(exporterEntries));
    curExporterEntries = 0;

	/**count columns*/
	numberOfColumns = 0;
	for(uint32_t i=0; identify[i].cname!=0; i++) numberOfColumns++;
	
	// build SQL INSERT statement
	ostringstream sql;
	sql << "INSERT INTO " << tableName << " (";
	for (uint32_t i=0; i<numberOfColumns; i++) {
		sql << identify[i].cname;
		if (i<numberOfColumns-1) sql << ", ";
	}
	sql << ") VALUES ";

	/**Initialize structure members Statement*/
	insertBuffer.curRows = 0;
	insertBuffer.maxRows = maxStatements;
	insertBuffer.sql = new char[(INS_WIDTH+3)*(numberOfColumns+1)*maxStatements+sql.str().size()+1];
	strcpy(insertBuffer.sql, sql.str().c_str());
	insertBuffer.appendPtr = insertBuffer.sql+sql.str().size();
	insertBuffer.bodyPtr = insertBuffer.sql+sql.str().size();

	connectToDB();
}

/**
 * Frees memory used by an IpfixDbWriterPg
 * @param IpfixDbWriterPg handle obtained by calling @c createipfixDbWriter()
 */
IpfixDbWriterPg::~IpfixDbWriterPg()
{
	writeToDb();
	if (conn) PQfinish(conn);

	delete[] insertBuffer.sql;	
}

#endif
