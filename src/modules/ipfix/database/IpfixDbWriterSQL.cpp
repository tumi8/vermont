/*
 * IPFIX Database Writer Base for SQL-based Databases
 * Copyright (C) 2012 Lothar Braun
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

/* Some constants that are common to IpfixDbWriterSQL and IpfixDbReader */

#include "IpfixDbWriterSQL.hpp"
#include "common/msg.h"
#include "common/Misc.h"
#include "common/Time.h"

#include <stdexcept>
#include <string.h>
#include <stdlib.h>
#include <sstream>
#include <algorithm>

using namespace std;

#define EXPORTERID 0


/***** Structs for SQL databases *********************************************/


/** MySQL **/

/**
 *	struct to identify column names with IPFIX_TYPEID an the dataType to store in database
 *	ExporterID is no IPFIX_TYPEID, its user specified
 *      Attention: order of entries is important!
 */
const static IpfixDbWriterSQL::Column identifyMySQL [] = {
	{	CN_dstIP, 		IPFIX_TYPEID_destinationIPv4Address,	"INTEGER(10) UNSIGNED", 	0, 0},
	{	CN_srcIP, 		IPFIX_TYPEID_sourceIPv4Address,		"INTEGER(10) UNSIGNED", 	0, 0},
	{	CN_srcPort, 		IPFIX_TYPEID_sourceTransportPort,	"SMALLINT(5) UNSIGNED", 	0, 0},
	{	CN_dstPort, 		IPFIX_TYPEID_destinationTransportPort,	"SMALLINT(5) UNSIGNED", 	0, 0},
	{	CN_proto, 		IPFIX_TYPEID_protocolIdentifier,	"TINYINT(3) UNSIGNED", 		0, 0 },
	{	CN_dstTos, 		IPFIX_TYPEID_classOfServiceIPv4,	"TINYINT(3) UNSIGNED", 		0, 0},
	{	CN_bytes, 		IPFIX_TYPEID_octetDeltaCount,		"BIGINT(20) UNSIGNED", 		0, 0},
	{	CN_pkts, 		IPFIX_TYPEID_packetDeltaCount,		"BIGINT(20) UNSIGNED", 		0, 0},
	{	CN_firstSwitched, 	IPFIX_TYPEID_flowStartMilliSeconds,	"BIGINT(15) UNSIGNED", 	0, 0}, // default value is invalid/not used for this ent
	{	CN_lastSwitched, 	IPFIX_TYPEID_flowEndMilliSeconds,	"BIGINT(15) UNSIGNED", 	0, 0}, // default value is invalid/not used for this entry
	{	CN_tcpControlBits,  	IPFIX_TYPEID_tcpControlBits,		"SMALLINT(5) UNSIGNED", 	0, 0},
	//TODO: use enterprise number for the following extended types (Gerhard, 12/2009)
	{	CN_revbytes, 		IPFIX_TYPEID_octetDeltaCount,		"BIGINT(20) UNSIGNED",		IPFIX_PEN_reverse, 0},
	{	CN_revpkts, 		IPFIX_TYPEID_packetDeltaCount,		"BIGINT(20) UNSIGNED", 		IPFIX_PEN_reverse, 0},
	{	CN_revFirstSwitched, 	IPFIX_TYPEID_flowStartMilliSeconds,	"BIGINT(15) UNSIGNED", 	IPFIX_PEN_reverse, 0}, // default value is invalid/not used for this entry
	{	CN_revLastSwitched, 	IPFIX_TYPEID_flowEndMilliSeconds,	"BITINT(15 UNSIGNED", 	IPFIX_PEN_reverse, 0}, // default value is invalid/not used for this entry
	{	CN_revTcpControlBits,  IPFIX_TYPEID_tcpControlBits,		"SMALLINT(5) UNSIGNED", 	IPFIX_PEN_reverse, 0},
	{	CN_maxPacketGap,	IPFIX_ETYPEID_maxPacketGap,		"BIGINT(20) UNSIGNED", 		IPFIX_PEN_vermont|IPFIX_PEN_reverse},
	{	CN_exporterID, 		EXPORTERID,				"SMALLINT(5) UNSIGNED", 	0, 0},
	{	CN_flowStartSysUpTime,	IPFIX_TYPEID_flowStartSysUpTime,	"INTEGER(10) UNSIGNED",		0, 0},
	{	CN_flowEndSysUpTime,	IPFIX_TYPEID_flowEndSysUpTime,		"INTEGER(10) UNSIGNED",		0, 0},
	{	0	} // last entry must be 0
};

/** Postgres **/

/**
 *	struct to identify column names with IPFIX_TYPEID an the dataType to store in database
 *	ExporterID is no IPFIX_TYPEID, its user specified
 *      Attention: order of entries is important!
 */
const static IpfixDbWriterSQL::Column identifyPg [] = {
	{	CN_srcIP, 		IPFIX_TYPEID_sourceIPv4Address,		"bigint", 0, 0},
	{	CN_dstIP, 		IPFIX_TYPEID_destinationIPv4Address,	"bigint", 0, 0},
	{	CN_srcPort,		IPFIX_TYPEID_sourceTransportPort,	"integer", 0, 0},
	{	CN_dstPort,		IPFIX_TYPEID_destinationTransportPort,	"integer",0, 0},
	{	CN_proto,		IPFIX_TYPEID_protocolIdentifier,	"smallint", 0, 0},
	{	CN_dstTos,		IPFIX_TYPEID_classOfServiceIPv4,	"smallint", 0, 0},
	{	CN_bytes,		IPFIX_TYPEID_octetDeltaCount,		"bigint", 0, 0},
	{	CN_pkts,		IPFIX_TYPEID_packetDeltaCount,		"bigint", 0, 0},
	{	CN_firstSwitched,	IPFIX_TYPEID_flowStartMilliSeconds,	"bigint", 0, 0}, // default value is invalid/not used for this entry
	{	CN_lastSwitched,	IPFIX_TYPEID_flowEndMilliSeconds,	"bigint", 0, 0}, // default value is invalid/not used for this entry
	{	CN_tcpControlBits,	IPFIX_TYPEID_tcpControlBits,		"smallint", 0, 0},
	{	CN_revbytes,		IPFIX_TYPEID_octetDeltaCount,		"bigint", IPFIX_PEN_reverse, 0},
	{	CN_revpkts,		IPFIX_TYPEID_packetDeltaCount,		"bigint", IPFIX_PEN_reverse, 0},
	{	CN_revFirstSwitched,	IPFIX_TYPEID_flowStartMilliSeconds,	"bigint", IPFIX_PEN_reverse, 0}, // default value is invalid/not used for this entry
	{	CN_revLastSwitched,	IPFIX_TYPEID_flowEndMilliSeconds,	"bigint", IPFIX_PEN_reverse, 0}, // default value is invalid/not used for this entry
	{	CN_revTcpControlBits,	IPFIX_TYPEID_tcpControlBits,		"smallint", IPFIX_PEN_reverse, 0},
	{	CN_maxPacketGap,	IPFIX_ETYPEID_maxPacketGap,		"bigint", IPFIX_PEN_vermont|IPFIX_PEN_reverse, 0},
	{	CN_revMaxPacketGap,	IPFIX_ETYPEID_maxPacketGap,		"bigint", IPFIX_PEN_vermont|IPFIX_PEN_reverse, 0},
	{	CN_exporterID,		EXPORTERID, 				"integer", 0, 0},
	{	0} // last entry must be 0
};



/***** Global Variables ******************************************************/

/**
 * maximum length of one item in a SQL statement
 */
const uint16_t MAX_COL_LENGTH = 22;

/**
 * save given elements of record to database
 */
void IpfixDbWriterSQL::processDataDataRecord(IpfixRecord::SourceID* sourceID,
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
void IpfixDbWriterSQL::onDataRecord(IpfixDataRecord* record)
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

bool IpfixDbWriterSQL::checkCurrentTable(uint64_t flowStart)
{
	return curTable.timeStart!=0 && (curTable.timeStart<=flowStart && curTable.timeEnd>flowStart);
}

string IpfixDbWriterSQL::getTimeAsString(uint64_t milliseconds, const char* formatstring, bool addfraction, uint32_t microseconds)
{
	char timebuffer[40];
	struct tm date;
	time_t seconds = milliseconds/1000; // round down to start of interval and convert ms -> s
	gmtime_r(&seconds, &date);
	strftime(timebuffer, ARRAY_SIZE(timebuffer), formatstring, &date);
	if (addfraction) {
#if DEBUG
		if (ARRAY_SIZE(timebuffer)-strlen(timebuffer)<7) THROWEXCEPTION("IpfixDbWriterSQL: buffer size too small");
#endif
		snprintf(timebuffer+strlen(timebuffer), 8, ".%06u", static_cast<uint32_t>(milliseconds%1000)*1000+(microseconds%1000));
	}
	return string(timebuffer);
}

bool IpfixDbWriterSQL::setCurrentTable(uint64_t flowStart)
{
	if (insertBuffer.curRows) THROWEXCEPTION("programming error: setCurrentTable MUST NOT be called when entries are still cached!");
	string tablename = tablePrefix;

	uint64_t starttime = (flowStart/TABLE_INTERVAL)*TABLE_INTERVAL; // round down to start of interval
	uint64_t endtime = starttime+TABLE_INTERVAL;

	tablename += getTimeAsString(starttime, "_%y%m%d_%H%M%S", false);

	if (!createDBTable(tablename.c_str(), starttime, endtime)) return false;

	string sql = getInsertString(tablename);

	strcpy(insertBuffer.sql, sql.c_str());
	insertBuffer.bodyPtr = insertBuffer.sql + sql.size();
	insertBuffer.appendPtr = insertBuffer.bodyPtr;

	curTable.name = tablename;
	curTable.timeStart = starttime;
	curTable.timeEnd = endtime;
	return true;
}


// extract seconds, ms and ys from ntp time
void IpfixDbWriterSQL::extractNtp64(uint64_t& intdata, uint32_t& micros)
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
void IpfixDbWriterSQL::fillInsertRow(IpfixRecord::SourceID* sourceID,
		TemplateInfo* dataTemplateInfo, uint16_t length, IpfixRecord::Data* data)
{
	uint64_t intdata = 0;
	uint64_t flowstart = 0;
	uint64_t intdata2 = 0;
	uint32_t k;
	bool notfound, notfound2;
	bool first = true;
	ostringstream rowStream;

	rowStream << "(";

	/**loop over the columname and loop over the IPFIX_TYPEID of the record
	 to get the corresponding data to store and make insert statement*/
	for(vector<Column>::iterator col = tableColumns.begin(); col != tableColumns.end(); col++) {
		if (col->ipfixId == EXPORTERID) {
			intdata = (uint64_t)getExporterID(sourceID);
		} else {
			notfound = true;
			// try to gather data required for the field
			if(dataTemplateInfo->fieldCount > 0) {
				// look inside the ipfix record
				for(k=0; k < dataTemplateInfo->fieldCount; k++) {
					if(dataTemplateInfo->fieldInfo[k].type.enterprise ==  col->enterprise && dataTemplateInfo->fieldInfo[k].type.id == col->ipfixId) {
						notfound = false;
						intdata = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset));
						DPRINTF("IpfixDbWriter::getdata: really saw ipfix id %d in packet with intdata %llX, type %d, length %d and offset %X", col->ipfixId, intdata, dataTemplateInfo->fieldInfo[k].type.id, dataTemplateInfo->fieldInfo[k].type.length, dataTemplateInfo->fieldInfo[k].offset);
						break;
					}
				}
			}
			if( dataTemplateInfo->dataCount > 0 && notfound) {
				// look in static data fields of template for data
				for(k=0; k < dataTemplateInfo->dataCount; k++) {
					if(dataTemplateInfo->fieldInfo[k].type.enterprise == col->enterprise && dataTemplateInfo->dataInfo[k].type.id == col->ipfixId) {
						notfound = false;
						intdata = getdata(dataTemplateInfo->dataInfo[k].type,(dataTemplateInfo->data+dataTemplateInfo->dataInfo[k].offset));
						break;
					}
				}
			}
			if(notfound) {
				notfound2 = true;
				// for some Ids, we have an alternative
				if(col->enterprise == 0) {
					switch (col->ipfixId) {
						case IPFIX_TYPEID_flowStartSeconds:
							if(dataTemplateInfo->fieldCount > 0) {
								for(k=0; k < dataTemplateInfo->fieldCount; k++) {
									// look for alternative (flowStartMilliSeconds/1000)
									if(dataTemplateInfo->fieldInfo[k].type.id == IPFIX_TYPEID_flowStartMilliSeconds) {
										intdata = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset));
										notfound = false;
										break;
									}
									// if no flow start time is available, maybe this is is from a netflow from Cisco
									// then - as a last alternative - use flowStartSysUpTime as flow start time
									if(dataTemplateInfo->fieldInfo[k].type.id == IPFIX_TYPEID_flowStartSysUpTime) {
										intdata2 = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset)) * 1000;
										notfound2 = false;
									}
								}
								if(notfound && !notfound2) {
									intdata = intdata2;
									notfound = false;
								}
							}
							break;
						case IPFIX_TYPEID_flowEndSeconds:
							if(dataTemplateInfo->fieldCount > 0) {
								for(k=0; k < dataTemplateInfo->fieldCount; k++) {
									// look for alternative (flowEndMilliSeconds/1000)
									if(dataTemplateInfo->fieldInfo[k].type.id == IPFIX_TYPEID_flowEndMilliSeconds) {
										intdata = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset));
										notfound = false;
										break;
									}
									// if no flow end time is available, maybe this is is from a netflow from Cisco
									// then use flowEndSysUpTime as flow start time
									if(dataTemplateInfo->fieldInfo[k].type.id == IPFIX_TYPEID_flowEndSysUpTime) {
										intdata2 = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset)) * 1000;
										notfound2 = false;
									}
								}
								if(notfound && !notfound2) {
									intdata = intdata2;
									notfound = false;
								}
							}
							break;
					}
				} else if (col->enterprise==IPFIX_PEN_reverse) {
					switch (col->ipfixId) {
						case IPFIX_TYPEID_flowStartSeconds:
							// look for alternative (revFlowStartMilliSeconds/1000)
							if(dataTemplateInfo->fieldCount > 0) {
								for(k=0; k < dataTemplateInfo->fieldCount; k++) {
									if(dataTemplateInfo->fieldInfo[k].type == InformationElement::IeInfo(IPFIX_TYPEID_flowStartMilliSeconds, IPFIX_PEN_reverse)) {
										intdata = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset));
										notfound = false;
										break;
									}
								}
							}
							break;
						case IPFIX_TYPEID_flowEndSeconds:
							// look for alternative (revFlowEndMilliSeconds/1000)
							if(dataTemplateInfo->fieldCount > 0) {
								for(k=0; k < dataTemplateInfo->fieldCount; k++) {
									if(dataTemplateInfo->fieldInfo[k].type == InformationElement::IeInfo(IPFIX_TYPEID_flowEndMilliSeconds, IPFIX_PEN_reverse)) {
										intdata = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset));
										notfound = false;
										break;
									}
								}
							}
							break;

					}
				}
				// if still not found, get default value
				if(notfound)
					intdata = col->defaultValue;
			}

			// we need extra treatment for timing related fields
			if(col->enterprise == 0 ) {
				switch (col->ipfixId) {
					case IPFIX_TYPEID_flowStartSeconds:
						// save time for table access
						if (flowstart==0) flowstart = intdata;
						break;

					case IPFIX_TYPEID_flowEndSeconds:
						break;

					case IPFIX_TYPEID_flowStartMilliSeconds:
						// if flowStartSeconds is not stored in one of the columns, but flowStartMilliSeconds is,
						// then we use flowStartMilliSeconds for table access
						// This is realized by storing this value only if flowStartSeconds has not yet been seen.
						// A later appearing flowStartSeconds will override this value.
						if (flowstart==0)
							flowstart = intdata;
					case IPFIX_TYPEID_flowEndMilliSeconds:
						// in the database the millisecond entry is counted from last second
						//intdata %= 1000;
						break;
				}
			} else if (col->enterprise==IPFIX_PEN_reverse)
				switch (col->ipfixId) {
					case IPFIX_TYPEID_flowStartMilliSeconds:
					case IPFIX_TYPEID_flowEndMilliSeconds:
						// in the database the millisecond entry is counted from last second
						//intdata %= 1000;
						break;
				}
		}

		DPRINTF("saw ipfix id %d in packet with intdata %llX", col->ipfixId, intdata);

		if(first)
			rowStream << intdata;
		else
			rowStream << "," << intdata;
		first = false;
	}

	rowStream << "),";

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

	strcat(insertBuffer.appendPtr, rowStream.str().c_str());
	insertBuffer.appendPtr += rowStream.str().size();
	insertBuffer.curRows++;
}


/**
 *	Get data of the record is given by the IPFIX_TYPEID
 */
uint64_t IpfixDbWriterSQL::getdata(InformationElement::IeInfo type, IpfixRecord::Data* data)
{
	if(type.id == IPFIX_TYPEID_sourceIPv4Address || type.id == IPFIX_TYPEID_destinationIPv4Address) {
		return getipv4address(type, data);
	} else {
		return getIPFIXValue(type, data);
	}
}
/**
 *	determine the ipv4address of the data record
 */
uint32_t IpfixDbWriterSQL::getipv4address( InformationElement::IeInfo type, IpfixRecord::Data* data)
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
uint64_t IpfixDbWriterSQL::getIPFIXValue(InformationElement::IeInfo type, IpfixRecord::Data* data)
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
uint32_t IpfixDbWriterSQL::getdefaultIPFIXdata(int ipfixtype_id)
{
	int i;
	for( i=0; tableColumns[i].cname != 0; i++) {
		if(ipfixtype_id == tableColumns[i].ipfixId) {
			return tableColumns[i].defaultValue;
		}
	}
	return 0;
}

string IpfixDbWriterSQL::getInsertString(string tableName)
{
	ostringstream sql;
	sql  << "INSERT INTO " << tableName << " (";
	for (uint32_t i = 0; i < numberOfColumns; ++i) {
		sql << tableColumns[i].cname;
		if (i < numberOfColumns - 1) sql << ",";
	}
	sql << ") VALUES ";
	return sql.str();
}

/***** Exported Functions ****************************************************/

/**
 * Creates a new IpfixDbWriterSQL. Do not forget to call @c startipfixDbWriter() to begin writing to Database
 * @return handle to use when calling @c destroyipfixDbWriter()
 */
IpfixDbWriterSQL::IpfixDbWriterSQL(const char* dbtype, const char* host, const char* db,
		const char* user, const char* pw,
		unsigned int port, uint16_t observationDomainId,
		int maxStatements, vector<string> columns)
{
	/**Initialize structure members IpfixDbWriterSQL*/
	hostName = host;
	dbName = db;
	userName = user;
	password = pw;
	portNum = port;
	dbType = dbtype;
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
	
	if (dbType == "mysql") {
		identify = (Column*)identifyMySQL;
	} else if (dbType == "postgres") {
		identify = (Column*)identifyPg;
	} else {
		THROWEXCEPTION("IpfixDbWriter: dbType \"%s\" is not implemented!", dbType.c_str());
	}

	if(columns.empty())
		THROWEXCEPTION("IpfixDbWriter: cannot initiate with no columns");
	
	/* get columns */
	bool first = true;
	for(vector<string>::const_iterator col = columns.begin(); col != columns.end(); col++) {
		int i = 0;
		while(identify[i].cname != 0) {
			if(col->compare(identify[i].cname) == 0) {
				Column c = identify[i];
				tableColumns.push_back(c);
				// update tableColumnsString
				if(!first)
					tableColumnsString.append(",");
				tableColumnsString.append(identify[i].cname);
				// update tableColumnsCreateString
				if(!first)
					tableColumnsCreateString.append(", ");
				tableColumnsCreateString.append(identify[i].cname);
				tableColumnsCreateString.append(" ");
				tableColumnsCreateString.append(identify[i].dataType);
				first = false;
				break;
			}
			i++;
		}
	}
	msg(MSG_INFO, "IpfixDbWriter: columns are %s", tableColumnsString.c_str());


	/**count columns*/
	numberOfColumns = tableColumns.size();

	/**Initialize structure members Statement*/
	insertBuffer.curRows = 0;
	insertBuffer.maxRows = maxStatements;
	insertBuffer.sql = new char[(INS_WIDTH+3)*(numberOfColumns+1)*maxStatements+numberOfColumns*20+60+1];
	*insertBuffer.sql = 0;
}

/**
 * Frees memory used by an IpfixDbWriterSQL
 * @param IpfixDbWriterSQL handle obtained by calling @c createipfixDbWriter()
 */
IpfixDbWriterSQL::~IpfixDbWriterSQL()
{
	delete[] insertBuffer.sql;
}
