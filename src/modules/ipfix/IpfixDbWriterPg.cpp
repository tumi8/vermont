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
 *	struct to identify column names with IPFIX_TYPEID an the dataType to store in database
 *	ExporterID is no IPFIX_TYPEID, its user specified
 *      Attention: order of entries is important!
 */
const IpfixDbWriterPg::Column IpfixDbWriterPg::identify [] = {
	{	"srcIP", IPFIX_TYPEID_sourceIPv4Address, "inet", 0},
	{	"dstIP", IPFIX_TYPEID_destinationIPv4Address, "inet", 0},
	{	"srcPort", IPFIX_TYPEID_sourceTransportPort, "integer", 0},
	{	"dstPort", IPFIX_TYPEID_destinationTransportPort, "integer",0},
	{	"proto",IPFIX_TYPEID_protocolIdentifier , "smallint", 0},
	{	"dstTos", IPFIX_TYPEID_classOfServiceIPv4, "smallint", 0},
	{	"bytes", IPFIX_TYPEID_octetDeltaCount, "bigint", 0},
	{	"pkts", IPFIX_TYPEID_packetDeltaCount, "bigint", 0},
	{	"firstSwitched", IPFIX_TYPEID_flowStartMilliSeconds, "timestamp", 0}, // default value is invalid/not used for this entry
	{	"lastSwitched", IPFIX_TYPEID_flowEndMilliSeconds, "timestamp", 0}, // default value is invalid/not used for this entry
	{	"tcpControlBits", IPFIX_TYPEID_tcpControlBits,  "smallint", 0},
	{	"revbytes", IPFIX_ETYPEID_revOctetDeltaCount, "bigint", 0},
	{	"revpkts", IPFIX_ETYPEID_revPacketDeltaCount, "bigint", 0},
	{	"revFirstSwitched", IPFIX_ETYPEID_revFlowStartMilliSeconds, "timestamp", 0}, // default value is invalid/not used for this entry
	{	"revLastSwitched", IPFIX_ETYPEID_revFlowEndMilliSeconds, "timestamp", 0}, // default value is invalid/not used for this entry
	{	"revTcpControlBits", IPFIX_ETYPEID_revTcpControlBits,  "smallint", 0},
	{	"maxPacketGap", IPFIX_ETYPEID_maxPacketGap,  "bigint", 0},
	{	"revMaxPacketGap", IPFIX_ETYPEID_revMaxPacketGap,  "bigint", 0},
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
			ctsql << identify[i].cname << " " << identify[i].dataType;
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
		cpsql << getTimeAsString(starttime, "%Y-%m-%d %H:%M:%S", true);
		cpsql << "' AND firstswitched<'" << getTimeAsString(endtime, "%Y-%m-%d %H:%M:%S", true);
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
 * save given elements of record to database
 */
void IpfixDbWriterPg::processDataDataRecord(IpfixRecord::SourceID* sourceID,
		TemplateInfo* dataTemplateInfo, uint16_t length,
		IpfixRecord::Data* data)
{
	DPRINTF("Processing data record");

	if (dbError) {
		connectToDB();
		if (dbError) return;
	}

	/** check if statement buffer is not full*/
	if(insertBuffer.curRows==insertBuffer.maxRows) {
		msg(MSG_ERROR, "failed to write data to database, trying again ...");
		if (!writeToDb()) {
			msg(MSG_ERROR, "dropping record");
			return;
		}
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
	// only treat non-Options Data Records (although we cannot be sure that there is a Flow inside)
	if((record->templateInfo->setId != TemplateInfo::NetflowTemplate) 
		&& (record->templateInfo->setId != TemplateInfo::IpfixTemplate) 
		&& (record->templateInfo->setId != TemplateInfo::IpfixDataTemplate)) {
		record->removeReference();
		return;
	}

	processDataDataRecord(record->sourceID.get(), record->templateInfo.get(),
			record->dataLength, record->data);

	record->removeReference();
}

bool IpfixDbWriterPg::checkCurrentTable(uint64_t flowStart)
{
	return curTable.timeStart!=0 && (curTable.timeStart<=flowStart && curTable.timeEnd>flowStart);
}

string IpfixDbWriterPg::getTimeAsString(uint64_t milliseconds, const char* formatstring, bool addfraction, uint32_t microseconds)
{
	char timebuffer[40];
	struct tm date;
	time_t seconds = milliseconds/1000; // round down to start of interval and convert ms -> s
	gmtime_r(&seconds, &date);
	strftime(timebuffer, ARRAY_SIZE(timebuffer), formatstring, &date);
	if (addfraction) {
#if DEBUG
		if (ARRAY_SIZE(timebuffer)-strlen(timebuffer)<7) THROWEXCEPTION("IpfixDbWriterPg: buffer size too small");
#endif
		snprintf(timebuffer+strlen(timebuffer), 8, ".%06u", static_cast<uint32_t>(milliseconds%1000)*1000+(microseconds%1000));
	}
	return string(timebuffer);
}

bool IpfixDbWriterPg::setCurrentTable(uint64_t flowStart)
{
	if (insertBuffer.curRows) THROWEXCEPTION("programming error: setCurrentTable MUST NOT be called when entries are still cached!");
	string tablename = tablePrefix;

	uint64_t starttime = (flowStart/TABLE_INTERVAL)*TABLE_INTERVAL; // round down to start of interval
	uint64_t endtime = starttime+TABLE_INTERVAL;

	tablename += getTimeAsString(starttime, "_%y%m%d_%H%M%S", false);

	if (!createDBTable(tablename.c_str(), starttime, endtime)) return false;

	// build SQL INSERT statement
	ostringstream sql;
	sql << "INSERT INTO " << tablename << " (";
	for (uint32_t i=0; i<numberOfColumns; i++) {
		sql << identify[i].cname;
		if (i<numberOfColumns-1) sql << ", ";
	}
	sql << ") VALUES ";
	strcpy(insertBuffer.sql, sql.str().c_str());
	insertBuffer.bodyPtr = insertBuffer.sql + sql.str().size();
	insertBuffer.appendPtr = insertBuffer.bodyPtr;

	curTable.name = tablename;
	curTable.timeStart = starttime;
	curTable.timeEnd = endtime;
	return true;
}


// extract seconds, ms and ys from ntp time
void IpfixDbWriterPg::extractNtp64(uint64_t& intdata, uint32_t& micros)
{
	if (intdata==0) {
		micros = 0;
		return;
	}
	timeval t = timentp64(*((ntp64*)(&intdata)));
	intdata = (uint64_t)t.tv_sec*1000+t.tv_usec/1000;
	micros = t.tv_usec%1000;
}


/**
 *	loop over the TemplateInfo (fieldinfo,datainfo) to get the IPFIX values to store in database
 *  results are stored in insertBuffer.sql
 */
void IpfixDbWriterPg::fillInsertRow(IpfixRecord::SourceID* sourceID,
		TemplateInfo* dataTemplateInfo, uint16_t length, IpfixRecord::Data* data)
{
	uint32_t j, k;
	uint64_t intdata = 0;
	ostringstream insertsql;
	uint64_t flowstart = 0;

	insertsql << "(";

	/**loop over the columname and loop over the IPFIX_TYPEID of the record
	 to get the corresponding data to store */
	for( j=0; j<numberOfColumns; j++) {
		bool notfound = true;
		uint32_t microseconds = 0; // special case for handle of microseconds (will not be contained in intdata)

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
								} else if (dataTemplateInfo->fieldInfo[k].type.id == IPFIX_TYPEID_flowStartNanoSeconds) {
									intdata = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset));
									extractNtp64(intdata, microseconds);
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
								} else if (dataTemplateInfo->fieldInfo[k].type.id == IPFIX_ETYPEID_revFlowStartNanoSeconds) {
									intdata = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset));
									extractNtp64(intdata, microseconds);
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
								} else if (dataTemplateInfo->fieldInfo[k].type.id == IPFIX_TYPEID_flowEndNanoSeconds) {
									intdata = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset));
									extractNtp64(intdata, microseconds);
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
								} else if (dataTemplateInfo->fieldInfo[k].type.id == IPFIX_ETYPEID_revFlowEndNanoSeconds) {
									intdata = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset));
									extractNtp64(intdata, microseconds);
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
				insertsql << "'" << IPToString(ntohl(intdata)) << "'";
				break;

			// convert integer to timestamps
			case IPFIX_TYPEID_flowStartMilliSeconds:
				// save time for table access
				if (flowstart==0) flowstart = intdata;
			case IPFIX_TYPEID_flowEndMilliSeconds:
			case IPFIX_ETYPEID_revFlowStartMilliSeconds:
			case IPFIX_ETYPEID_revFlowEndMilliSeconds:
				insertsql << "'" << getTimeAsString(intdata, "%Y-%m-%d %H:%M:%S", true, microseconds) << "'";
				break;

			// all other integer data is directly converted to a string
			default:
				insertsql << intdata;
				break;
		}
		if (j!=numberOfColumns-1) insertsql << ",";

	}

	insertsql << "),";


	// if this flow belongs to a different table, flush all cached entries now
	// and get new table
	if (!checkCurrentTable(flowstart)) {
		if (!writeToDb()) {
			msg(MSG_ERROR, "failed to flush table, dropping record");
			return;
		}
		if (!setCurrentTable(flowstart)) {
			msg(MSG_ERROR, "failed to change table, dropping record");
			return;
		}
	}


	strcat(insertBuffer.appendPtr, insertsql.str().c_str());
	insertBuffer.appendPtr += insertsql.str().size();
	insertBuffer.curRows++;
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

/**
 *	Get data of the record is given by the IPFIX_TYPEID
 */
uint64_t IpfixDbWriterPg::getdata(InformationElement::IeInfo type, IpfixRecord::Data* data)
{
	if(type.id == IPFIX_TYPEID_sourceIPv4Address || type.id == IPFIX_TYPEID_destinationIPv4Address)
	return getipv4address(type, data);
	else
	return getIPFIXValue(type, data);
}
/**
 *	determine the ipv4address of the data record
 */
uint32_t IpfixDbWriterPg::getipv4address( InformationElement::IeInfo type, IpfixRecord::Data* data)
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
uint64_t IpfixDbWriterPg::getIPFIXValue(InformationElement::IeInfo type, IpfixRecord::Data* data)
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
    bzero(&exporterEntries, sizeof(exporterEntries));
    curExporterEntries = 0;
    curTable.timeStart = 0;
    curTable.timeEnd = 0;
    curTable.name = "";
    tablePrefix = "f"; // TODO: make this in config file configurable!

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
