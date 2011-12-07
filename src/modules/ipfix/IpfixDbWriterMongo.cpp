/*
 * IPFIX Database Writer MongoDB Connector
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

#ifdef MONGO_SUPPORT_ENABLED

#include <stdexcept>
#include <string.h>
#include <iomanip>
#include <stdlib.h>
#include <vector>
#include "IpfixDbWriterMongo.hpp"
#include "common/msg.h"

IpfixDbWriterMongo::Property identify [] = {
	{CN_dstIP, 		"NUMBER(10)", 	0, IPFIX_TYPEID_destinationIPv4Address, 0},
	{CN_srcIP, 		"NUMBER(10)", 	0, IPFIX_TYPEID_sourceIPv4Address, 0},
	{CN_srcPort, 		"NUMBER(5)", 	0, IPFIX_TYPEID_sourceTransportPort, 0},
	{CN_dstPort, 		"NUMBER(5)", 	0, IPFIX_TYPEID_destinationTransportPort, 0},
	{CN_proto, 		"NUMBER(3)", 		0, IPFIX_TYPEID_protocolIdentifier, 0 },
	{CN_dstTos, 		"NUMBER(3)", 		0, IPFIX_TYPEID_classOfServiceIPv4, 0},
	{CN_bytes, 		"NUMBER(20)", 		0, IPFIX_TYPEID_octetDeltaCount, 0},
	{CN_pkts, 		"NUMBER(20)", 		0, IPFIX_TYPEID_packetDeltaCount, 0},
	{CN_firstSwitched, 	"NUMBER(10)", 	0, IPFIX_TYPEID_flowStartSeconds, 0}, // default value is invalid/not used for this ent
	{CN_lastSwitched, 	"NUMBER(10)", 	0, IPFIX_TYPEID_flowEndSeconds, 0}, // default value is invalid/not used for this entry
	{CN_firstSwitchedMillis, "NUMBER(5)", 	0, IPFIX_TYPEID_flowStartMilliSeconds, 0},
	{CN_lastSwitchedMillis, "NUMBER(5)", 	0, IPFIX_TYPEID_flowEndMilliSeconds, 0},
	{CN_tcpControlBits,  	"NUMBER(5)", 	0, IPFIX_TYPEID_tcpControlBits, 0},
	//TODO: use enterprise number for the following extended types (Gerhard, 12/2009)
	{CN_revbytes, 		"NUMBER(20)", 		0, IPFIX_TYPEID_octetDeltaCount, IPFIX_PEN_reverse},
	{CN_revpkts, 		"NUMBER(20)", 		0, IPFIX_TYPEID_packetDeltaCount, IPFIX_PEN_reverse},
	{CN_revFirstSwitched, 	"NUMBER(10)", 	0, IPFIX_TYPEID_flowStartSeconds, IPFIX_PEN_reverse}, // default value is invalid/not used for this entry
	{CN_revLastSwitched, 	"NUMBER(10)", 	0, IPFIX_TYPEID_flowEndSeconds, IPFIX_PEN_reverse}, // default value is invalid/not used for this entry
	{CN_revFirstSwitchedMillis, "NUMBER(5)", 	0, IPFIX_TYPEID_flowStartMilliSeconds, IPFIX_PEN_reverse},
	{CN_revLastSwitchedMillis, "NUMBER(5)", 	0, IPFIX_TYPEID_flowEndMilliSeconds, IPFIX_PEN_reverse},
	{CN_revTcpControlBits,  "NUMBER(5)", 	0, IPFIX_TYPEID_tcpControlBits, IPFIX_PEN_reverse},
	{CN_maxPacketGap,  	"NUMBER(20)", 		0, IPFIX_ETYPEID_maxPacketGap, IPFIX_PEN_vermont|IPFIX_PEN_reverse},
	{CN_exporterID, 	"NUMBER(5)", 	0, EXPORTERID, 0},
	{0} // last entry must be 0
};

/**
 * Compare two source IDs and check if exporter is the same (i.e., same IP address and observationDomainId
 */
bool IpfixDbWriterMongo::equalExporter(const IpfixRecord::SourceID& a, const IpfixRecord::SourceID& b) {
	return (a.observationDomainId == b.observationDomainId) &&
		(a.exporterAddress.len == b.exporterAddress.len) &&
		(memcmp(a.exporterAddress.ip, b.exporterAddress.ip, a.exporterAddress.len) == 0 );
}

/**
 * (re)connect to database
 */
int IpfixDbWriterMongo::connectToDB()
{
	dbError = true;
	
	// If a connection exists don't reconnect
	if (con) return 0;
  
  // Connect
  string err;
  mongo::HostAndPort dbLogon;
  dbLogon = mongo::HostAndPort::HostAndPort(dbHost, dbPort);
  msg(MSG_INFO,"IpfixDbWriterMongo: Connection details: %s", dbLogon.toString().c_str());
  con.connect(dbLogon, &err);
  if(err)
	{
		msg(MSG_FATAL,"IpfixDbWriterMongo: Mongo connect failed. Error: %s", err.c_str());
		return 1;
	}
    
  if(dbUser && dbPassword) 
  {
    // we need to authenticate
    con.auth(dbName, dbUser, dbPassword, &err);
    if(err)
	  {
		  msg(MSG_FATAL,"IpfixDbWriterMongo: Mongo authentication failed. Error: %s", err.c_str());
		  return 1;
	  }
  }

	msg(MSG_DEBUG,"IpfixDbWriterMongo: Oracle connection successful");
	dbError = false;
	return 0;
}

// FIXME 
/**
 * save record to database
 */
void IpfixDbWriterMongo::processDataDataRecord(const IpfixRecord::SourceID& sourceID,
		TemplateInfo& dataTemplateInfo, uint16_t length,
		IpfixRecord::Data* data)
{
	string rowString;
	time_t flowStartSeconds;
	msg(MSG_DEBUG, "IpfixDbWriter: Processing data record");

	if (dbError) {
		msg(MSG_DEBUG, "IpfixDbWriter: reconnecting to DB");
		connectToDB();
		if (dbError) return;
	}

	/* get new insert */
	if(srcId.observationDomainId != 0) {
		// use default source id
		rowString = getInsertString(rowString, flowStartSeconds, srcId, dataTemplateInfo, length, data);
	} else {
		rowString = getInsertString(rowString, flowStartSeconds, sourceID, dataTemplateInfo, length, data);
	}
	msg(MSG_DEBUG, "IpfixDbWriter: Row: %s", rowString.c_str());
	

	// if current table is not ok, write to db and get new table name
	if(!(flowStartSeconds >= currentTable.startTime && flowStartSeconds <= currentTable.endTime)) {
		if(numberOfInserts > 0) {
			msg(MSG_DEBUG, "IpfixDbWriter: Writing buffered records to database");
			insertStatement << " SELECT * FROM dual";
			writeToDb();
			numberOfInserts = 0;
		}
		if (setCurrentTable(flowStartSeconds) != 0) {
			return;
		}
	}


	// start new insert statement if necessary
	if (numberOfInserts == 0) {
		// start insert statement
		insertStatement.str("");
		insertStatement.clear();
		insertStatement << "INSERT ALL INTO " << currentTable.name << " (" << documentPropertiesString << ") VALUES " << rowString;
		numberOfInserts = 1;
	} else {
		// append insert statement
		insertStatement << " INTO " << currentTable.name << " (" << documentPropertiesString << ") VALUES " << rowString;
		numberOfInserts++;
	}

	// write to db if maxInserts is reached
	if(numberOfInserts == maxInserts) {
		msg(MSG_DEBUG, "IpfixDbWriter: Writing buffered records to database");
		insertStatement << " SELECT * FROM dual";
		writeToDb();
		numberOfInserts = 0;
	}
}

// FIXME
/**
 *	loop over table columns and template to get the IPFIX values in correct order to store in database
 *	The result is written into row, the firstSwitched time is returned in flowstartsec
 */
mongo::BSONObj& IpfixDbWriterMongo::getInsertObj(string& row, time_t& flowstartsec, const IpfixRecord::SourceID& sourceID,
		TemplateInfo& dataTemplateInfo,uint16_t length, IpfixRecord::Data* data)
{
	uint64_t intdata = 0;
	uint64_t intdata2 = 0;
	uint32_t k;
	bool notfound, notfound2;
	bool first = true;
	ostringstream rowStream(row);

	flowstartsec = 0;
	rowStream << "(";

	/**loop over the columname and loop over the IPFIX_TYPEID of the record
	 to get the corresponding data to store and make insert statement*/
	for(vector<Column>::iterator col = documentProperties.begin(); col != documentProperties.end(); col++) {
		if (col->ipfixId == EXPORTERID) {
			// if this is the same source ID as last time, we get the exporter id from currentExporter
			if ((currentExporter != NULL) && equalExporter(sourceID, currentExporter->sourceID)) {
				DPRINTF("Exporter is same as last time (ODID=%d, id=%d)", sourceID.observationDomainId, currentExporter->id);
				intdata = (uint64_t)currentExporter->id;
			} else {
			// lookup exporter buffer to get exporterID from sourcID and expIp
				intdata = (uint64_t)getExporterID(sourceID);
			}
		} else {
			notfound = true;
			// try to gather data required for the field
			if(dataTemplateInfo.fieldCount > 0) {
				// look inside the ipfix record
				for(k=0; k < dataTemplateInfo.fieldCount; k++) {
					if(dataTemplateInfo.fieldInfo[k].type.enterprise ==  col->enterprise && dataTemplateInfo.fieldInfo[k].type.id == col->ipfixId) {
						notfound = false;
						intdata = getData(dataTemplateInfo.fieldInfo[k].type,(data+dataTemplateInfo.fieldInfo[k].offset));
						DPRINTF("IpfixDbWriterMongo::getData: really saw ipfix id %d in packet with intdata %llX, type %d, length %d and offset %X", col->ipfixId, intdata, dataTemplateInfo.fieldInfo[k].type.id, dataTemplateInfo.fieldInfo[k].type.length, dataTemplateInfo.fieldInfo[k].offset);
						break;
					}
				}
			}
			if( dataTemplateInfo.dataCount > 0 && notfound) {
				// look in static data fields of template for data
				for(k=0; k < dataTemplateInfo.dataCount; k++) {
					if(dataTemplateInfo.fieldInfo[k].type.enterprise == col->enterprise && dataTemplateInfo.dataInfo[k].type.id == col->ipfixId) {
						notfound = false;
						intdata = getData(dataTemplateInfo.dataInfo[k].type,(dataTemplateInfo.data+dataTemplateInfo.dataInfo[k].offset));
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
							if(dataTemplateInfo.fieldCount > 0) {
								for(k=0; k < dataTemplateInfo.fieldCount; k++) {
									// look for alternative (flowStartMilliSeconds/1000)
									if(dataTemplateInfo.fieldInfo[k].type.id == IPFIX_TYPEID_flowStartMilliSeconds) {
										intdata = getData(dataTemplateInfo.fieldInfo[k].type,(data+dataTemplateInfo.fieldInfo[k].offset)) / 1000;
										notfound = false;
										break;
									}
									// if no flow start time is available, maybe this is is from a netflow from Cisco
									// then - as a last alternative - use flowStartSysUpTime as flow start time
									if(dataTemplateInfo.fieldInfo[k].type.id == IPFIX_TYPEID_flowStartSysUpTime) {
										intdata2 = getData(dataTemplateInfo.fieldInfo[k].type,(data+dataTemplateInfo.fieldInfo[k].offset));
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
							if(dataTemplateInfo.fieldCount > 0) {
								for(k=0; k < dataTemplateInfo.fieldCount; k++) {
									// look for alternative (flowEndMilliSeconds/1000)
									if(dataTemplateInfo.fieldInfo[k].type.id == IPFIX_TYPEID_flowEndMilliSeconds) {
										intdata = getData(dataTemplateInfo.fieldInfo[k].type,(data+dataTemplateInfo.fieldInfo[k].offset)) / 1000;
										notfound = false;
										break;
									}
									// if no flow end time is available, maybe this is is from a netflow from Cisco
									// then use flowEndSysUpTime as flow start time
									if(dataTemplateInfo.fieldInfo[k].type.id == IPFIX_TYPEID_flowEndSysUpTime) {
										intdata2 = getData(dataTemplateInfo.fieldInfo[k].type,(data+dataTemplateInfo.fieldInfo[k].offset));
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
							if(dataTemplateInfo.fieldCount > 0) {
								for(k=0; k < dataTemplateInfo.fieldCount; k++) {
									if(dataTemplateInfo.fieldInfo[k].type == InformationElement::IeInfo(IPFIX_TYPEID_flowStartMilliSeconds, IPFIX_PEN_reverse)) {
										intdata = getData(dataTemplateInfo.fieldInfo[k].type,(data+dataTemplateInfo.fieldInfo[k].offset)) / 1000;
										notfound = false;
										break;
									}
								}
							}
							break;
						case IPFIX_TYPEID_flowEndSeconds:
							// look for alternative (revFlowEndMilliSeconds/1000)
							if(dataTemplateInfo.fieldCount > 0) {
								for(k=0; k < dataTemplateInfo.fieldCount; k++) {
									if(dataTemplateInfo.fieldInfo[k].type == InformationElement::IeInfo(IPFIX_TYPEID_flowEndMilliSeconds, IPFIX_PEN_reverse)) {
										intdata = getData(dataTemplateInfo.fieldInfo[k].type,(data+dataTemplateInfo.fieldInfo[k].offset)) / 1000;
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
						if (flowstartsec==0) flowstartsec = intdata;
						break;

					case IPFIX_TYPEID_flowEndSeconds:
						break;

					case IPFIX_TYPEID_flowStartMilliSeconds:
						// if flowStartSeconds is not stored in one of the columns, but flowStartMilliSeconds is,
						// then we use flowStartMilliSeconds for table access
						// This is realized by storing this value only if flowStartSeconds has not yet been seen.
						// A later appearing flowStartSeconds will override this value.
						if (flowstartsec==0)
							flowstartsec = intdata/1000;
					case IPFIX_TYPEID_flowEndMilliSeconds:
						// in the database the millisecond entry is counted from last second
						intdata %= 1000;
						break;
				}
			} else if (col->enterprise==IPFIX_PEN_reverse)
				switch (col->ipfixId) {
					case IPFIX_TYPEID_flowStartMilliSeconds:
					case IPFIX_TYPEID_flowEndMilliSeconds:
						// in the database the millisecond entry is counted from last second
						intdata %= 1000;
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

	if (flowstartsec == 0) {
		msg(MSG_ERROR, "IpfixDbWriterMongo: Failed to get timing data from record. Will be saved in default table.");
	}

	row = rowStream.str();
	DPRINTF("Insert row: %s", row.c_str());
	return row;
}


// FIXME
/*
 * Write Objects to database
 */
int IpfixDbWriterMongo::writeToDb()
{
	return 1; //error
}

// FIXME
/**
 *	Returns the id of the exporter collection entry or 0 in the case of an error
 */
int IpfixDbWriterMongo::getExporterID(const IpfixRecord::SourceID& sourceID)
{
	list<ExporterCacheEntry>::iterator iter;
	int id = -1;
	uint32_t expIp = 0;

	iter = exporterCache.begin();
	while(iter != exporterCache.end()) {
		if (equalExporter(iter->sourceID, sourceID)) {
			// found exporter in exporterCache
			DPRINTF("Exporter (ODID=%d, id=%d) found in exporter cache", sourceID.observationDomainId, iter->id);
			exporterCache.push_front(*iter);
			exporterCache.erase(iter);
			// update current exporter
			currentExporter = &exporterCache.front();
			return exporterCache.front().id;
		}
		iter++;
	}

	// convert IP address (correct host byte order since 07/2010)
	expIp = sourceID.exporterAddress.toUInt32();

	// search exporter collection
	// sql << "SELECT id FROM exporter WHERE sourceID=" << sourceID.observationDomainId << " AND srcIp=" << expIp;
	// msg(MSG_DEBUG, "IpfixDbWriterMongo: SQL Query: %s", sql.str().c_str());
	
  // insert new entry in exporter table since it is not found
	if(id == -1)
	{
		//sql << "INSERT INTO exporter (ID,sourceID,srcIP) VALUES ( 0 ,'" << sourceID.observationDomainId << "','" << expIp << "')";
		//msg(MSG_DEBUG, "IpfixDbWriterMongo: SQL Query: %s", sql.str().c_str());
	}
	// insert exporter in cache
	ExporterCacheEntry tmp = {sourceID, id};
	exporterCache.push_front(tmp);

	// update current exporter
	currentExporter = &exporterCache.front();

	// pop last element if exporter cache is to long
	if(exporterCache.size() > MAX_EXPORTER)
		exporterCache.pop_back();

	return id;
}

/**
 *	Get data of the record is given by the IPFIX_TYPEID
 */
uint64_t IpfixDbWriterMongo::getData(InformationElement::IeInfo type, IpfixRecord::Data* data)
{
	switch (type.length) {
		case 1:
			return (*(uint8_t*)data);
		case 2:
			return ntohs(*(uint16_t*)data);
		case 4:
			return ntohl(*(uint32_t*)data);
		case 5:	// may occur in the case if IP address + mask
			return ntohl(*(uint32_t*)data);
		case 8:
			return ntohll(*(uint64_t*)data);
		default:
			printf("Uint with length %d unparseable", type.length);
			return 0;
	}
}

/***** Public Methods ****************************************************/

/**
 * called on Data Record arrival
 */
void IpfixDbWriterMongo::onDataRecord(IpfixDataRecord* record)
{
	// only treat non-Options Data Records (although we cannot be sure that there is a Flow inside)
	if((record->templateInfo->setId != TemplateInfo::NetflowTemplate)
		&& (record->templateInfo->setId != TemplateInfo::IpfixTemplate)
		&& (record->templateInfo->setId != TemplateInfo::IpfixDataTemplate)) {
		record->removeReference();
		return;
	}

	msg(MSG_DEBUG, "IpfixDbWriterMongo: Data record received will be passed for processing");
	processDataDataRecord(*record->sourceID.get(), *record->templateInfo.get(),
			record->dataLength, record->data);

	record->removeReference();
}

/**
 * Constructor
 */
IpfixDbWriterMongo::IpfixDbWriterMongo(const string& hostname, const string& database,
		const string& username, const string& password,
		unsigned port, uint32_t observationDomainId, unsigned maxStatements,
		const vector<string>& propertyNames)
	: currentExporter(NULL), numberOfInserts(0), maxInserts(maxStatements),
	dbHost(hostname), dbName(database), dbUser(username), dbPassword(password), dbPort(port), con(0)
{
	int i;

	// set default source id
	srcId.exporterAddress.len = 0;
	srcId.observationDomainId = observationDomainId;
	srcId.exporterPort = 0;
	srcId.receiverPort = 0;
	srcId.protocol = 0;
	srcId.fileDescriptor = 0;

	if(propertyNames.empty())
		THROWEXCEPTION("IpfixDbWriterMongo: cannot initiate with no properties");

	/* get properties */
	bool first = true;
	for(vector<string>::const_iterator prop = propertyNames.begin(); prop != propertyNames.end(); prop++) {
		i = 0;
		while(identify[i].propertyName != 0) {
			if(prop->compare(identify[i].propertyName) == 0) {
				Column c = identify[i];
				documentProperties.push_back(c);
				// update documentPropertiesString
				if(!first)
					documentPropertiesString.append(",");
				documentPropertiesString.append(identify[i].propertyName);
				// update documentPropertiesCreateString
				if(!first)
					documentPropertiesCreateString.append(", ");
				documentPropertiesCreateString.append(identify[i].propertyName);
				documentPropertiesCreateString.append(" ");
				documentPropertiesCreateString.append(identify[i].propertyType);
				first = false;
				break;
			}
			i++;
		}
	}
	msg(MSG_INFO, "IpfixDbWriterMongo: properties are %s", .c_str());

	if(connectToDB() != 0)
		THROWEXCEPTION("IpfixDbWriterMongo creation failed");
}


/**
 * Destructor
 */
IpfixDbWriterMongo::~IpfixDbWriterMongo()
{
	writeToDb();
}



#endif /* MONGO_SUPPORT_ENABLED */
