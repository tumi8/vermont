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
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

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


/****** Methods **************************************************************/
// NOTE: This function can not be made virtual and moved to a sub-class since
// it is called in the IpfixDbWriterSQL constructor
std::string IpfixDbWriterSQL::getDBDataType(const uint16_t ipfixType)
{
	if	(dbType == "mysql") {
		switch (ipfixType) {

		case IPFIX_TYPE_octetArray:
		case IPFIX_TYPE_basicList:
		case IPFIX_TYPE_subTemplateList:
		case IPFIX_TYPE_subTemplateMultiList:
			return "BLOB";		// max is 65,535 bytes

		case IPFIX_TYPE_unsigned8:
			return "TINYINT UNSIGNED";

		case IPFIX_TYPE_signed8:
			return "TINYINT";

		case IPFIX_TYPE_unsigned16:
			return "SMALLINT UNSIGNED";

		case IPFIX_TYPE_signed16:
			return "SMALLINT";

		case IPFIX_TYPE_unsigned32:
		case IPFIX_TYPE_dateTimeSeconds:
		case IPFIX_TYPE_ipv4Address:
			return "INT UNSIGNED";

		case IPFIX_TYPE_signed32:
			return "INT";

		case IPFIX_TYPE_unsigned64:
		case IPFIX_TYPE_dateTimeMilliseconds:
		case IPFIX_TYPE_dateTimeMicroseconds:
		case IPFIX_TYPE_dateTimeNanoseconds:
		case IPFIX_TYPE_macAddress:
			return "BIGINT UNSIGNED";

		case IPFIX_TYPE_signed64:
			return "BIGINT";

		case IPFIX_TYPE_float32:
			return "FLOAT";

		case IPFIX_TYPE_float64:
			return "DOUBLE";

		case IPFIX_TYPE_boolean:
			return "BOOLEAN";

		case IPFIX_TYPE_string:
			return "TEXT";		// max is 65,535 characters

		case IPFIX_TYPE_ipv6Address:
			return "BINARY(16)";

		default:
			THROWEXCEPTION("IpfixDbWriter: Type with non matching length %d ", ipfixType);
		}
	} else if (dbType == "postgres") {
		// postgres does not do unsigned types. we therefore use the bigger field. this wastes 
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
		switch (ipfixType) {

		case IPFIX_TYPE_octetArray:
		case IPFIX_TYPE_basicList:
		case IPFIX_TYPE_subTemplateList:
		case IPFIX_TYPE_subTemplateMultiList:
			return "BLOB";

		case IPFIX_TYPE_unsigned8:
			return "NUMBER(3)";

		case IPFIX_TYPE_signed8:
			return "NUMBER(4)";		// one digit for sign

		case IPFIX_TYPE_unsigned16:
			return "NUMBER(5)";

		case IPFIX_TYPE_signed16:
			return "NUMBER(6)";		// one digit for sign

		case IPFIX_TYPE_unsigned32:
		case IPFIX_TYPE_dateTimeSeconds:
		case IPFIX_TYPE_ipv4Address:
			return "NUMBER(10)";

		case IPFIX_TYPE_signed32:
			return "NUMBER(11)";	// one digit for sign

		case IPFIX_TYPE_unsigned64:
		case IPFIX_TYPE_signed64:
		case IPFIX_TYPE_dateTimeMilliseconds:
		case IPFIX_TYPE_dateTimeMicroseconds:
		case IPFIX_TYPE_dateTimeNanoseconds:
			return "NUMBER(20)";

		case IPFIX_TYPE_float32:
			return "BINARY_FLOAT";

		case IPFIX_TYPE_float64:
			return "BINARY_DOUBLE";

		case IPFIX_TYPE_boolean:
			return "NUMBER(1)";

		case IPFIX_TYPE_macAddress:
			return "NUMBER(15)";

		case IPFIX_TYPE_string:
			return "CLOB";

		case IPFIX_TYPE_ipv6Address:
			return "NUMBER(38)";

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
	DPRINTF_INFO("Processing data record");


	if (dbError) {
		connectToDB();
		if (dbError) return;
	}

	/** check if statement buffer is not full*/
	if(insertBuffer.curRows==insertBuffer.maxRows) {
		msg(LOG_ERR, "failed to write data to database, trying again ...");
		if (!writeToDb()) {
			msg(LOG_ERR, "dropping record");
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
		msg(LOG_NOTICE, "Writing buffered records to database");
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
		&& (record->templateInfo->setId != TemplateInfo::IpfixTemplate)) {
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
	uint64_t flowstart = 0;
	uint32_t k;
	bool first = true;
	ostringstream rowStream;

	rowStream << "(";

	/**loop over the columname and loop over the IPFIX_TYPEID of the record
	 to get the corresponding data to store and make insert statement*/
	for(vector<Column>::iterator col = tableColumns.begin(); col != tableColumns.end(); col++) {
		// Reset parsedData after each iteration
		string parsedData = "";

		if (col->ipfixId == EXPORTERID) {
			parsedData = boost::str(boost::format("%d") % getExporterID(sourceID));
			break;
		} else {
			// try to gather data required for the field
			// look inside the ipfix record
			for(k=0; k < dataTemplateInfo->fieldCount; k++) {
				if(dataTemplateInfo->fieldInfo[k].type.enterprise == col->enterprise && dataTemplateInfo->fieldInfo[k].type.id == col->ipfixId) {
					parseIpfixData(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset), &parsedData);
					DPRINTF_INFO("IpfixDbWriter::parseIpfixData: really saw ipfix id %d (%s) in packet with parsedData %s, type %d, length %d and offset %X", col->ipfixId, ipfix_id_lookup(col->ipfixId, col->enterprise)->name, parsedData.c_str(), dataTemplateInfo->fieldInfo[k].type.id, dataTemplateInfo->fieldInfo[k].type.length, dataTemplateInfo->fieldInfo[k].offset);
					break;
				}
			}
			// check for time-related alternative fields in the database
			if (parsedData.empty()) {
				checkTimeAlternatives(&(*col), dataTemplateInfo, data, &parsedData);
			}

			// get default value if nothing found until now
			if (parsedData.empty()) {
					parsedData = boost::lexical_cast<int>(col->defaultValue);
			}

			// we need to extract the flow start time for determining the correct DB table
			if(col->enterprise == 0 ) {
				switch (col->ipfixId) {
					case IPFIX_TYPEID_flowStartSeconds:
						// save time for table access
						flowstart = boost::lexical_cast<uint64_t>(parsedData) * 1000;
						break;

					case IPFIX_TYPEID_flowStartMilliseconds:
						// if flowStartSeconds is not stored in one of the columns, but flowStartMilliseconds is,
						// then we use flowStartMilliseconds for table access
						// This is realized by storing this value only if flowStartSeconds has not yet been seen.
						// A later appearing flowStartSeconds will override this value.
						if (flowstart == 0) {
							flowstart = boost::lexical_cast<uint64_t>(parsedData);
						}
				}
			} else if (col->enterprise == IPFIX_PEN_reverse)
				switch (col->ipfixId) {
					case IPFIX_TYPEID_flowStartMilliseconds:
					case IPFIX_TYPEID_flowEndMilliseconds:
						if (flowstart == 0) {
							flowstart = boost::lexical_cast<uint64_t>(parsedData);
						}
						break;
				}
		}

		DPRINTF_INFO("saw ipfix id %d in packet with parsedData %s", col->ipfixId, parsedData.c_str());

		if(first) {
			rowStream << parsedData;
		} else {
			rowStream << "," << parsedData;
		}
		first = false;
	}

	rowStream << ")";

	// if this flow belongs to a different table, flush all cached entries now
	// and get new table
	if (!checkCurrentTable(flowstart)) {
		if (insertBuffer.curRows != 0 && !writeToDb()) {
			msg(LOG_ERR, "failed to flush table, dropping record");
			return;
		}
		if (!setCurrentTable(flowstart)) {
			msg(LOG_ERR, "failed to change table, dropping record");
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
 * Check alternatives if no exact was found for time-related IPFIX IEs.
 */
void IpfixDbWriterSQL::checkTimeAlternatives(Column* col, TemplateInfo* dataTemplateInfo, IpfixRecord::Data* data, string* parsedData) {

	int k;

	// for some Ids, we have an alternative
	if(col->enterprise == 0) {
		switch (col->ipfixId) {
			case IPFIX_TYPEID_flowStartSeconds:
				for(k=0; k < dataTemplateInfo->fieldCount; k++) {
					// look for alternative (flowStartMilliseconds/1000)
					if(dataTemplateInfo->fieldInfo[k].type.id == IPFIX_TYPEID_flowStartMilliseconds) {
						parseUintAndScale(dataTemplateInfo->fieldInfo[k], data, .001, parsedData);
						break;
					}
					// if no flow start time is available, maybe this is is from a netflow from Cisco
					// then - as a last alternative - use flowStartSysUpTime as flow start time
					if(dataTemplateInfo->fieldInfo[k].type.id == IPFIX_TYPEID_flowStartSysUpTime) {
						parseUintAndScale(dataTemplateInfo->fieldInfo[k], data, 1., parsedData);
					}
				}
				break;
			case IPFIX_TYPEID_flowStartMilliseconds:
				for(k=0; k < dataTemplateInfo->fieldCount; k++) {
					// look for alternative (flowStartSeconds*1000)
					if(dataTemplateInfo->fieldInfo[k].type.id == IPFIX_TYPEID_flowStartSeconds) {
						parseUintAndScale(dataTemplateInfo->fieldInfo[k], data, 1000., parsedData);
						break;
					}
					// if no flow start time is available, maybe this is is from a netflow from Cisco
					// then - as a last alternative - use flowStartSysUpTime as flow start time
					if(dataTemplateInfo->fieldInfo[k].type.id == IPFIX_TYPEID_flowStartSysUpTime) {
						parseUintAndScale(dataTemplateInfo->fieldInfo[k], data, 1000., parsedData);
					}
				}
				break;
			case IPFIX_TYPEID_flowEndSeconds:
				for(k=0; k < dataTemplateInfo->fieldCount; k++) {
					// look for alternative (flowEndMilliseconds/1000)
					if(dataTemplateInfo->fieldInfo[k].type.id == IPFIX_TYPEID_flowEndMilliseconds) {
						parseUintAndScale(dataTemplateInfo->fieldInfo[k], data, .001, parsedData);
						break;
					}
					// if no flow end time is available, maybe this is is from a netflow from Cisco
					// then use flowEndSysUpTime as flow start time
					if(dataTemplateInfo->fieldInfo[k].type.id == IPFIX_TYPEID_flowEndSysUpTime) {
						parseUintAndScale(dataTemplateInfo->fieldInfo[k], data, 1., parsedData);
					}
				}
				break;
			case IPFIX_TYPEID_flowEndMilliseconds:
				for(k=0; k < dataTemplateInfo->fieldCount; k++) {
					// look for alternative (flowEndSeconds*1000)
					if(dataTemplateInfo->fieldInfo[k].type.id == IPFIX_TYPEID_flowEndSeconds) {
						parseUintAndScale(dataTemplateInfo->fieldInfo[k], data, 1000., parsedData);
						break;
					}
					// if no flow end time is available, maybe this is is from a netflow from Cisco
					// then use flowEndSysUpTime as flow start time
					if(dataTemplateInfo->fieldInfo[k].type.id == IPFIX_TYPEID_flowEndSysUpTime) {
						parseUintAndScale(dataTemplateInfo->fieldInfo[k], data, 1000., parsedData);
					}
				}
				break;
		}
	} else if (col->enterprise == IPFIX_PEN_reverse) {
		switch (col->ipfixId) {
			case IPFIX_TYPEID_flowStartSeconds:
				// look for alternative (revFlowStartMilliseconds/1000)
				for(k=0; k < dataTemplateInfo->fieldCount; k++) {
					if(dataTemplateInfo->fieldInfo[k].type == InformationElement::IeInfo(IPFIX_TYPEID_flowStartMilliseconds, IPFIX_PEN_reverse)) {
						parseUintAndScale(dataTemplateInfo->fieldInfo[k], data, .001, parsedData);
						break;
					}
				}
			break;
			case IPFIX_TYPEID_flowStartMilliseconds:
				// look for alternative (revFlowStartSeconds*1000)
				for(k=0; k < dataTemplateInfo->fieldCount; k++) {
					if(dataTemplateInfo->fieldInfo[k].type == InformationElement::IeInfo(IPFIX_TYPEID_flowStartSeconds, IPFIX_PEN_reverse)) {
						parseUintAndScale(dataTemplateInfo->fieldInfo[k], data, 1000., parsedData);
						break;
					}
				}
				break;
			case IPFIX_TYPEID_flowEndSeconds:
				// look for alternative (revFlowEndMilliseconds/1000)
				for(k=0; k < dataTemplateInfo->fieldCount; k++) {
					if(dataTemplateInfo->fieldInfo[k].type == InformationElement::IeInfo(IPFIX_TYPEID_flowEndMilliseconds, IPFIX_PEN_reverse)) {
						parseUintAndScale(dataTemplateInfo->fieldInfo[k], data, .001, parsedData);
						break;
					}
				}
				break;
			case IPFIX_TYPEID_flowEndMilliseconds:
				// look for alternative (revFlowEndSeconds*1000)
				for(k=0; k < dataTemplateInfo->fieldCount; k++) {
					if(dataTemplateInfo->fieldInfo[k].type == InformationElement::IeInfo(IPFIX_TYPEID_flowEndSeconds, IPFIX_PEN_reverse)) {
						parseUintAndScale(dataTemplateInfo->fieldInfo[k], data, 1000., parsedData);
						break;
					}
				}
		}
	}
}


/**
 * Parses a unsigned integer and scales it by factor, useful for checkTimeAlternatives().
 */
void IpfixDbWriterSQL::parseUintAndScale(TemplateInfo::FieldInfo fieldInfo, IpfixRecord::Data* data, double factor, string* parsedData) {

	parseIpfixData(fieldInfo.type, (data+fieldInfo.offset), parsedData);
	if (factor != 1) {
		*parsedData = boost::lexical_cast<string>(boost::lexical_cast<uint64_t>(*parsedData) * factor);
	}
}


/**
 *	Get data of the record is given by the IPFIX_TYPEID
 */
void IpfixDbWriterSQL::parseIpfixData(InformationElement::IeInfo type, IpfixRecord::Data* data, string* parsedData)
{
    switch (ipfix_id_lookup(type.id, type.enterprise)->type) {

		// Handle reduced size encoding
		case IPFIX_TYPE_boolean:
		case IPFIX_TYPE_unsigned8:
		case IPFIX_TYPE_unsigned16:
		case IPFIX_TYPE_unsigned32:
		case IPFIX_TYPE_unsigned64:
		case IPFIX_TYPE_dateTimeSeconds:
		case IPFIX_TYPE_dateTimeMilliseconds:
        case IPFIX_TYPE_dateTimeMicroseconds: // Note: Must be interpreted as 64 bit double, with floating point after 32 bit, epoch starts at 1900
		case IPFIX_TYPE_dateTimeNanoseconds:  // see also http://www.ntp.org/ntpfaq/NTP-s-algo.htm#AEN1895
			parseIpfixUint(data, type.length, parsedData);
			break;

		// Handle reduced size encoding
		case IPFIX_TYPE_signed8:
		case IPFIX_TYPE_signed16:
		case IPFIX_TYPE_signed32:
		case IPFIX_TYPE_signed64:
			parseIpfixInt(data, type.length, parsedData);
			break;

		// Handle reduced size encoding
		case IPFIX_TYPE_float32:
		case IPFIX_TYPE_float64:
			parseIpfixFloat(data, type.length, parsedData);
            break;

		// MAC addresses are handled differnetly in different databases
		case IPFIX_TYPE_macAddress:
			parseIpfixMacAddress(data, parsedData);
			break;

		// IPv4 addresses are handled differently in different databases
		case IPFIX_TYPE_ipv4Address:
			parseIpfixIpv4Address(data, parsedData);
			break;

		// IPv6 addresses are handled differently in different databases
		case IPFIX_TYPE_ipv6Address:
			parseIpfixIpv6Address(data, parsedData);
			break;

		case IPFIX_TYPE_string:
			// Truncate string to length announced in record
			*parsedData = boost::str(boost::format("'%." + boost::lexical_cast<std::string>(type.length)  +  "s'") );
            break;

		case IPFIX_TYPE_octetArray:
		case IPFIX_TYPE_basicList:
		case IPFIX_TYPE_subTemplateList:
		case IPFIX_TYPE_subTemplateMultiList:
		default:
			msg(LOG_ERR, "failed to parse record data of type %hu", ipfix_id_lookup(type.id, type.enterprise)->type);
    }
}

/**
 * Writes IPFIX unsigned integer into string, useful for handling reduced size encoding.
 */
void IpfixDbWriterSQL::parseIpfixUint(IpfixRecord::Data* data, uint16_t length, string* parsedData) {
	uint64_t acc = 0;

	for (int i = 0; i < length; i++) {
		acc = (acc << 8) + (uint8_t) data[i];
	}

	*parsedData = boost::lexical_cast<std::string>(acc);
}

/**
 * Writes IPFIX signed integer into string, useful for handling reduced size encoding.
 */
void IpfixDbWriterSQL::parseIpfixInt(IpfixRecord::Data* data, uint16_t length, string* parsedData) {
	// First byte is parsed as signed int8_t to preserve the sign
	int64_t acc = (int8_t) data[0];

	for (int i = 1; i < length; i++) {
		acc = (acc << 8) + (uint8_t) data[i];
	}

	*parsedData = boost::lexical_cast<std::string>(acc);
}

/**
 * Writes IPFIX 32 and 64 bit floats into string, useful for handling reduced size encoding.
 */
void IpfixDbWriterSQL::parseIpfixFloat(IpfixRecord::Data* data, uint16_t length, string* parsedData) {
	switch(length) {
		case 4:
			*parsedData = boost::lexical_cast<std::string>(*(float*) data);
			break;
		case 8:
			*parsedData = boost::lexical_cast<std::string>(*(double*) data);
			break;
		default:
			msg(LOG_ERR, "failed to parse float of length %hu", length);
	}
}

/**
 *	if there no IPFIX_TYPEID in the given data record
 * 	get the default value to store in the database columns
 */
uint32_t IpfixDbWriterSQL::getdefaultIPFIXdata(int ipfixtype_id)
{
	uint32_t i;
	for (i=0; i != tableColumns.size(); i++) {
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
		uint16_t ipfixId = 0;
		uint32_t enterpriseId = 0;
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
	msg(LOG_NOTICE, "IpfixDbWriter: columns are %s", tableColumnsString.c_str());


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

