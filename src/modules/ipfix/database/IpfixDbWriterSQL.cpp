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

	string sql = getInsertString();

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
						DPRINTF("IpfixDbWriterSQL::getRecData: really saw ipfix id %d in packet with intdata %llX, type %d, length %d and offset %X", identify[j].ipfixId, intdata, dataTemplateInfo->fieldInfo[k].type.id, dataTemplateInfo->fieldInfo[k].type.length, dataTemplateInfo->fieldInfo[k].offset);
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
				if (identify[j].enterprise==0) {
					switch (identify[j].ipfixId) {
						case IPFIX_TYPEID_flowStartMilliSeconds:
							// look for alternative (flowStartMilliSeconds/1000)
							if(dataTemplateInfo->fieldCount > 0) {
								for(k=0; k < dataTemplateInfo->fieldCount; k++) {
									if(dataTemplateInfo->fieldInfo[k].type == InformationElement::IeInfo(IPFIX_TYPEID_flowStartSeconds, 0)) {
										intdata = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset)) * 1000;
										notfound = false;
										break;
									} else if (dataTemplateInfo->fieldInfo[k].type == InformationElement::IeInfo(IPFIX_TYPEID_flowStartNanoSeconds, 0)) {
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
									if(dataTemplateInfo->fieldInfo[k].type == InformationElement::IeInfo(IPFIX_TYPEID_flowEndSeconds, 0)) {
										intdata = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset)) * 1000;
										notfound = false;
										break;
									} else if (dataTemplateInfo->fieldInfo[k].type == InformationElement::IeInfo(IPFIX_TYPEID_flowEndNanoSeconds, 0)) {
										intdata = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset));
										extractNtp64(intdata, microseconds);
										notfound = false;
										break;
									}
								}
							}
							break;
					}
				} else if (identify[j].enterprise==IPFIX_PEN_reverse) {
					switch (identify[j].ipfixId) {
						case IPFIX_TYPEID_flowStartMilliSeconds:
							// look for alternative (revFlowStartMilliSeconds/1000)
							if(dataTemplateInfo->fieldCount > 0) {
								for(k=0; k < dataTemplateInfo->fieldCount; k++) {
									if(dataTemplateInfo->fieldInfo[k].type == InformationElement::IeInfo(IPFIX_TYPEID_flowStartSeconds, IPFIX_PEN_reverse)) {
										intdata = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset)) * 1000;
										notfound = false;
										break;
									} else if (dataTemplateInfo->fieldInfo[k].type == InformationElement::IeInfo(IPFIX_TYPEID_flowStartNanoSeconds, IPFIX_PEN_reverse)) {
										intdata = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset));
										extractNtp64(intdata, microseconds);
										notfound = false;
										break;
									}
								}
							}
							break;

						case IPFIX_TYPEID_flowEndMilliSeconds:
							// look for alternative (revFlowEndMilliSeconds/1000)
							if(dataTemplateInfo->fieldCount > 0) {
								for(k=0; k < dataTemplateInfo->fieldCount; k++) {
									if(dataTemplateInfo->fieldInfo[k].type == InformationElement::IeInfo(IPFIX_TYPEID_flowEndSeconds, IPFIX_PEN_reverse)) {
										intdata = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset)) * 1000;
										notfound = false;
										break;
									} else if (dataTemplateInfo->fieldInfo[k].type == InformationElement::IeInfo(IPFIX_TYPEID_flowEndNanoSeconds, IPFIX_PEN_reverse)) {
										intdata = getdata(dataTemplateInfo->fieldInfo[k].type,(data+dataTemplateInfo->fieldInfo[k].offset));
										extractNtp64(intdata, microseconds);
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

		switch (identify[j].enterprise) {
			case 0:
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
						insertsql << "'" << getTimeAsString(intdata, "%Y-%m-%d %H:%M:%S", true, microseconds) << "'";
						break;

					// all other integer data is directly converted to a string
					default:
						insertsql << intdata;
						break;
				}
				break;

			case IPFIX_PEN_reverse:
				switch (identify[j].ipfixId) {
					case IPFIX_TYPEID_flowStartMilliSeconds:
					case IPFIX_TYPEID_flowEndMilliSeconds:
						insertsql << "'" << getTimeAsString(intdata, "%Y-%m-%d %H:%M:%S", true, microseconds) << "'";
						break;

					// all other integer data is directly converted to a string
					default:
						insertsql << intdata;
						break;
				}
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
 *	Get data of the record is given by the IPFIX_TYPEID
 */
uint64_t IpfixDbWriterSQL::getdata(InformationElement::IeInfo type, IpfixRecord::Data* data)
{
	if(type.id == IPFIX_TYPEID_sourceIPv4Address || type.id == IPFIX_TYPEID_destinationIPv4Address)
	return getipv4address(type, data);
	else
	return getIPFIXValue(type, data);
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
	for( i=0; identify[i].cname != 0; i++) {
		if(ipfixtype_id == identify[i].ipfixId) {
			return identify[i].defaultValue;
		}
	}
	return 0;
}

/***** Exported Functions ****************************************************/

/**
 * Creates a new IpfixDbWriterSQL. Do not forget to call @c startipfixDbWriter() to begin writing to Database
 * @return handle to use when calling @c destroyipfixDbWriter()
 */
IpfixDbWriterSQL::IpfixDbWriterSQL(const char* host, const char* db,
		const char* user, const char* pw,
		unsigned int port, uint16_t observationDomainId,
		int maxStatements)
{
	/**Initialize structure members IpfixDbWriterSQL*/
	hostName = host;
	dbName = db;
	userName = user;
	password = pw;
	portNum = port;
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
}

/**
 * Frees memory used by an IpfixDbWriterSQL
 * @param IpfixDbWriterSQL handle obtained by calling @c createipfixDbWriter()
 */
IpfixDbWriterSQL::~IpfixDbWriterSQL()
{
	deInit();
	delete[] insertBuffer.sql;
}

void IpfixDbWriterSQL::init()
{
	connectToDB();
}
void IpfixDbWriterSQL::deInit()
{
	writeToDb();
}

