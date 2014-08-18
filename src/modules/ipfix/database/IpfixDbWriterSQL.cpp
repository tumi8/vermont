/*
 * IPFIX Database Writer Base for SQL-based Databases
 * Copyright (C) 2012 Lothar Braun
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

/* Some constants that are common to IpfixDbWriterSQL and IpfixDbReader */

#if defined(DB_SUPPORT_ENABLED) || defined(MONGO_SUPPORT_ENABLED) || defined(PG_SUPPORT_ENABLED) || defined(ORACLE_SUPPORT_ENABLED) || defined(REDIS_SUPPORT_ENABLED)

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

const static IpfixDbWriterSQL::Column legacyNames [] = {
	{	CN_dstIP, 		IPFIX_TYPEID_destinationIPv4Address,	"", 0, 0},
	{	CN_srcIP, 		IPFIX_TYPEID_sourceIPv4Address,		"", 0, 0},
	{	CN_srcPort, 		IPFIX_TYPEID_sourceTransportPort,	"", 0, 0},
	{	CN_dstPort, 		IPFIX_TYPEID_destinationTransportPort,	"", 0, 0},
	{	CN_proto, 		IPFIX_TYPEID_protocolIdentifier,	"", 0, 0 },
	{	CN_dstTos, 		IPFIX_TYPEID_ipClassOfService,	"", 0, 0},
	{	CN_bytes, 		IPFIX_TYPEID_octetDeltaCount,		"", 0, 0},
	{	CN_pkts, 		IPFIX_TYPEID_packetDeltaCount,		"", 0, 0},
	{	CN_firstSwitched, 	IPFIX_TYPEID_flowStartMilliseconds,	"", 0, 0}, // default value is invalid/not used for this ent
	{	CN_lastSwitched, 	IPFIX_TYPEID_flowEndMilliseconds,	"", 0, 0}, // default value is invalid/not used for this entry
	{	CN_tcpControlBits,  	IPFIX_TYPEID_tcpControlBits,		"", 0, 0},
	//TODO: use enterprise number for the following extended types (Gerhard, 12/2009)
	{	CN_revbytes, 		IPFIX_TYPEID_octetDeltaCount,		"", IPFIX_PEN_reverse, 0},
	{	CN_revpkts, 		IPFIX_TYPEID_packetDeltaCount,		"", IPFIX_PEN_reverse, 0},
	{	CN_revFirstSwitched, 	IPFIX_TYPEID_flowStartMilliseconds,	"", IPFIX_PEN_reverse, 0}, // default value is invalid/not used for this entry
	{	CN_revLastSwitched, 	IPFIX_TYPEID_flowEndMilliseconds,	"", IPFIX_PEN_reverse, 0}, // default value is invalid/not used for this entry
	{	CN_revTcpControlBits,  IPFIX_TYPEID_tcpControlBits,		"", IPFIX_PEN_reverse, 0},
	{	CN_maxPacketGap,	IPFIX_ETYPEID_maxPacketGap,		"", IPFIX_PEN_vermont|IPFIX_PEN_reverse, 0},
	{	CN_exporterID, 		EXPORTERID,				"", 0, 0},
	{	CN_flowStartSysUpTime,	IPFIX_TYPEID_flowStartSysUpTime,	"", 0, 0},
	{	CN_flowEndSysUpTime,	IPFIX_TYPEID_flowEndSysUpTime,		"", 0, 0},
	{	"", 0, "", 0, 0	} // last entry must be 0
};


/***** Global Variables ******************************************************/

/**
 * maximum length of one item in a SQL statement
 */
const uint16_t MAX_COL_LENGTH = 22;


/****** Methods **************************************************************/
// NOTE: This function can not be made virtual and moved to a sub-class since
// it is called in the IpfixDbWriterSQL constructor
std::string IpfixDbWriterSQL::getDBDataType(const uint16_t ipfixType)
{
	if	(dbType == "mysql") {
		// TODO FIXME Adapt to new IPFIX type macros
		switch (ipfixType) {
		case 1:
			return "TINYINT UNSIGNED";
		case 2:
			return "SMALLINT UNSIGNED";
		case 4:
			return "INT UNSIGNED";
		case 8:
			return "BIGINT UNSIGNED";
		case 65535:
			// variable length, we only support fields up to 100 bytes (be careful, this may waste a lot of diskspace ...")
			return "VARCHAR(100)";
		default:
			THROWEXCEPTION("IpfixDbWriter: Type with non matching length %d ", ipfixType);
		}
	} else if (dbType == "postgres") {
		// TODO: postgres does not do unsigned types. we therefore use the bigger field. this wastes 
		/// disk space. Optimize! (except bigints ...)
		switch (ipfixType) {

		case IPFIX_TYPE_octetArray:
		case IPFIX_TYPE_basicList:
		case IPFIX_TYPE_subTemplateList:
		case IPFIX_TYPE_subTemplateMultiList:
			return "bytea";

		case IPFIX_TYPE_unsigned8:
		case IPFIX_TYPE_signed8:
		case IPFIX_TYPE_signed16:
			return "smallint";

		case IPFIX_TYPE_unsigned16:
		case IPFIX_TYPE_signed32:
		case IPFIX_TYPE_dateTimeSeconds:
			return "integer";

		case IPFIX_TYPE_unsigned32:
		case IPFIX_TYPE_unsigned64:
		case IPFIX_TYPE_signed64:
		case IPFIX_TYPE_dateTimeMilliseconds:
		case IPFIX_TYPE_dateTimeMicroseconds:
		case IPFIX_TYPE_dateTimeNanoseconds:
			return "bigint";

		case IPFIX_TYPE_float32:
			return "real";

		case IPFIX_TYPE_float64:
			return "double precision";

		case IPFIX_TYPE_boolean:
			return "boolean";

		case IPFIX_TYPE_macAddress:
			return "macaddr";

		case IPFIX_TYPE_string:
			return "text";

		case IPFIX_TYPE_ipv4Address:
		case IPFIX_TYPE_ipv6Address:
			return "inet";

		default:
			THROWEXCEPTION("IpfixDbWriter: Type with non matching length %d ", ipfixType);
		}	
	} else if (dbType == "oracle") {
		// TODO FIXME Adapt to new IPFIX type macros
		// TODO: someone should think about proper type lengths ...
		switch (ipfixType) {
		case 1:
			return "NUMBER(3)";
		case 2:
			return "NUMBER(6)";
		case 4:
			return "NUMBER(10)";
		case 8:
			return "NUMBER(20)";
		case 65535:
			// variable length, we only support fields up to 100 bytes (be careful, this may waste a lot of diskspace ...")
			return "VARCHAR(100)";
		default:
			THROWEXCEPTION("IpfixDbWriter: Type with non matching length %d ", ipfixType);
		}	
	}

 

	THROWEXCEPTION("IpfixDbWriter: Found unsupported database backend \"%s\". This is a programming error! Please contact vermont-dev@berlios.de!", dbType.c_str()); 
	// make compiler happy. we should never get here
	return "";
}


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

std::string IpfixDbWriterSQL::insertRowPrefix()
{
	if (insertBuffer.curRows > 0) {
		return ",";
	}
	return "";
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
						DPRINTF("IpfixDbWriter::getdata: really saw ipfix id %d (%s) in packet with intdata %llX, type %d, length %d and offset %X", col->ipfixId, ipfix_id_lookup(col->ipfixId, col->enterprise)->name, intdata, dataTemplateInfo->fieldInfo[k].type.id, dataTemplateInfo->fieldInfo[k].type.length, dataTemplateInfo->fieldInfo[k].offset);
						break;
					}
				}
			}
			if( dataTemplateInfo->dataCount > 0 && notfound) {
				// look in static data fields of template for data
				for(k=0; k < dataTemplateInfo->dataCount; k++) {
					if(dataTemplateInfo->dataInfo[k].type.enterprise == col->enterprise && dataTemplateInfo->dataInfo[k].type.id == col->ipfixId) {
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
									// look for alternative (flowStartMilliseconds/1000)
									if(dataTemplateInfo->fieldInfo[k].type.id == IPFIX_TYPEID_flowStartMilliseconds) {
										intdata = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset)) / 1000;
										notfound = false;
										break;
									}
									// if no flow start time is available, maybe this is is from a netflow from Cisco
									// then - as a last alternative - use flowStartSysUpTime as flow start time
									if(dataTemplateInfo->fieldInfo[k].type.id == IPFIX_TYPEID_flowStartSysUpTime) {
										intdata2 = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset));
										notfound2 = false;
									}
								}
								if(notfound && !notfound2) {
									intdata = intdata2;
									notfound = false;
								}
							}
							break;
						case IPFIX_TYPEID_flowStartMilliseconds:
							if(dataTemplateInfo->fieldCount > 0) {
								for(k=0; k < dataTemplateInfo->fieldCount; k++) {
									// look for alternative (flowStartSeconds*1000)
									if(dataTemplateInfo->fieldInfo[k].type.id == IPFIX_TYPEID_flowStartSeconds) {
										intdata = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset)) * 1000;
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
									// look for alternative (flowEndMilliseconds/1000)
									if(dataTemplateInfo->fieldInfo[k].type.id == IPFIX_TYPEID_flowEndMilliseconds) {
										intdata = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset)) / 1000;
										notfound = false;
										break;
									}
									// if no flow end time is available, maybe this is is from a netflow from Cisco
									// then use flowEndSysUpTime as flow start time
									if(dataTemplateInfo->fieldInfo[k].type.id == IPFIX_TYPEID_flowEndSysUpTime) {
										intdata2 = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset));
										notfound2 = false;
									}
								}
								if(notfound && !notfound2) {
									intdata = intdata2;
									notfound = false;
								}
							}
							break;
						case IPFIX_TYPEID_flowEndMilliseconds:
							if(dataTemplateInfo->fieldCount > 0) {
								for(k=0; k < dataTemplateInfo->fieldCount; k++) {
									// look for alternative (flowEndSeconds*1000)
									if(dataTemplateInfo->fieldInfo[k].type.id == IPFIX_TYPEID_flowEndSeconds) {
										intdata = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset)) * 1000;
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
							// look for alternative (revFlowStartMilliseconds/1000)
							if(dataTemplateInfo->fieldCount > 0) {
								for(k=0; k < dataTemplateInfo->fieldCount; k++) {
									if(dataTemplateInfo->fieldInfo[k].type == InformationElement::IeInfo(IPFIX_TYPEID_flowStartMilliseconds, IPFIX_PEN_reverse)) {
										intdata = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset)) / 1000;
										notfound = false;
										break;
									}
								}
							}
							break;
						case IPFIX_TYPEID_flowStartMilliseconds:
							// look for alternative (revFlowStartSeconds*1000)
							if(dataTemplateInfo->fieldCount > 0) {
								for(k=0; k < dataTemplateInfo->fieldCount; k++) {
									if(dataTemplateInfo->fieldInfo[k].type == InformationElement::IeInfo(IPFIX_TYPEID_flowStartSeconds, IPFIX_PEN_reverse)) {
										intdata = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset)) * 1000;
										notfound = false;
										break;
									}
								}
							}
							break;
						case IPFIX_TYPEID_flowEndSeconds:
							// look for alternative (revFlowEndMilliseconds/1000)
							if(dataTemplateInfo->fieldCount > 0) {
								for(k=0; k < dataTemplateInfo->fieldCount; k++) {
									if(dataTemplateInfo->fieldInfo[k].type == InformationElement::IeInfo(IPFIX_TYPEID_flowEndMilliseconds, IPFIX_PEN_reverse)) {
										intdata = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset)) / 1000;
										notfound = false;
										break;
									}
								}
							}
							break;
						case IPFIX_TYPEID_flowEndMilliseconds:
							// look for alternative (revFlowEndSeconds/1000)
							if(dataTemplateInfo->fieldCount > 0) {
								for(k=0; k < dataTemplateInfo->fieldCount; k++) {
									if(dataTemplateInfo->fieldInfo[k].type == InformationElement::IeInfo(IPFIX_TYPEID_flowEndSeconds, IPFIX_PEN_reverse)) {
										intdata = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset)) * 1000;
										notfound = false;
										break;
									}
								}
							}

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
						if (flowstart==0) flowstart = intdata * 1000;
						break;

					case IPFIX_TYPEID_flowEndSeconds:
						break;

					case IPFIX_TYPEID_flowStartMilliseconds:
						// if flowStartSeconds is not stored in one of the columns, but flowStartMilliseconds is,
						// then we use flowStartMilliseconds for table access
						// This is realized by storing this value only if flowStartSeconds has not yet been seen.
						// A later appearing flowStartSeconds will override this value.
						if (flowstart==0)
							flowstart = intdata;
					case IPFIX_TYPEID_flowEndMilliseconds:
						// in the database the millisecond entry is counted from last second
						//intdata %= 1000;
						break;
				}
			} else if (col->enterprise==IPFIX_PEN_reverse)
				switch (col->ipfixId) {
					case IPFIX_TYPEID_flowStartMilliseconds:
					case IPFIX_TYPEID_flowEndMilliseconds:
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

	rowStream << ")";

	// if this flow belongs to a different table, flush all cached entries now
	// and get new table
	if (!checkCurrentTable(flowstart)) {
		if (insertBuffer.curRows != 0 && !writeToDb()) {
			msg(MSG_ERROR, "failed to flush table, dropping record");
			return;
		}
		if (!setCurrentTable(flowstart)) {
			msg(MSG_ERROR, "failed to change table, dropping record");
			return;
		}
	}

	//  add prefix that needs to be applied before the actual values for
	// the insert string
	std::string prefix= insertRowPrefix();
	if (prefix.size() > 0) {
		strcat(insertBuffer.appendPtr, prefix.c_str());
		insertBuffer.appendPtr += prefix.size();
	}
	

	// add the rowStream buffer
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
	for( i=0; i != tableColumns.size(); i++) {
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
		int maxStatements, vector<string> columns, bool legacyNames, const char* prefix)
{
	/**Initialize structure members IpfixDbWriterSQL*/
	hostName = host;
	dbName = db;
	userName = user;
	password = pw;
	portNum = port;
	dbType = dbtype;
	socketName = 0;
	useLegacyNames = legacyNames;

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
	tablePrefix = prefix;
	
	legacyNamesMap = (Column*)::legacyNames;

	if(columns.empty())
		THROWEXCEPTION("IpfixDbWriter: cannot initiate with no columns");
	
	/* get columns */
	bool first = true;
	for(vector<string>::const_iterator col = columns.begin(); col != columns.end(); col++) {
		std::string columnName = *col;
		std::string dataType = "";
		uint16_t ipfixId;
		uint32_t enterpriseId;
		if (useLegacyNames) {
			bool found = false;
			int i = 0;
			while(legacyNamesMap[i].cname != "") {
				if(col->compare(legacyNamesMap[i].cname) == 0) {
					found = true;
					columnName = legacyNamesMap[i].cname;
					ipfixId = legacyNamesMap[i].ipfixId;
					enterpriseId = legacyNamesMap[i].enterprise;
					const struct ipfix_identifier* identifier = ipfix_id_lookup(ipfixId, enterpriseId);
					// special handling for ht exporter field
					if (ipfixId == 0) {
						dataType = getDBDataType(IPFIX_TYPE_unsigned16);
					} else {
						if (NULL == identifier) {
							THROWEXCEPTION("Programming error: Legacy Type name \"%s (id: %u)\" does not have a entry in ipfixlolib!", columnName.c_str(), ipfixId);
						}
						dataType = getDBDataType(identifier->type);
					}
				}
				i++;
			}
			if (!found) {
				THROWEXCEPTION("IpfixDbWriter: useLegacyNames was activated but name \"%s\" does not match any legacy name!", col->c_str());
			}
		} else {
			const struct ipfix_identifier* identifier = ipfix_name_lookup(col->c_str());
			if (NULL == identifier) {
				// only identifier that is not part of the standard IPFIX or VERMONT IPFIX name space is 
				// the exporterID field. We need to handle this manuallly.
				if (col->compare(CN_exporterID) == 0) {
					ipfixId = EXPORTERID;
					enterpriseId = 0;
					dataType = getDBDataType(IPFIX_TYPE_unsigned16);
				} else {
					THROWEXCEPTION("IpfixDbWriter: Could not find a matching IPFIX type for \"%s\". Cannot map the IPFIX type in IPFIX messages to the column names. If you think VERMONT should support type \"%s\", please contact the developers at vermont-dev@berlios.de", col->c_str(), col->c_str());
				}
			} else {
				ipfixId = identifier->id;
				enterpriseId = identifier->pen;
				dataType = getDBDataType(identifier->type);
			}
		}
		Column c;
		c.cname = columnName;
		c.ipfixId = ipfixId;
		c.enterprise = enterpriseId;
		c.dataType = dataType;
		c.defaultValue = 0;

		tableColumns.push_back(c);
		// update tableColumnsString
		if(!first)
			tableColumnsString.append(",");
		tableColumnsString.append(c.cname);
		// update tableColumnsCreateString
		if(!first)
			tableColumnsCreateString.append(", ");
		tableColumnsCreateString.append(c.cname);
		tableColumnsCreateString.append(" ");
		tableColumnsCreateString.append(c.dataType);
		first = false;
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

#endif

