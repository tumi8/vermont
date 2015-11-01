/*
 * IPFIX Database Writer MongoDB Connector
 * Copyright (C) 2011 Philipp Fehre <philipp.fehre@googlemail.com>
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

#ifdef MONGO_SUPPORT_ENABLED

#include <stdexcept>
#include <string.h>
#include <iomanip>
#include <stdlib.h>
#include <vector>
#include "IpfixDbWriterMongo.hpp"
#include "common/msg.h"

const IpfixDbWriterMongo::Property IpfixDbWriterMongo::identify [] = {
	{CN_dstIP, 0, IPFIX_TYPEID_destinationIPv4Address, 0},
	{CN_srcIP, 0, IPFIX_TYPEID_sourceIPv4Address, 0},
	{CN_srcPort, 0, IPFIX_TYPEID_sourceTransportPort, 0},
	{CN_dstPort, 0, IPFIX_TYPEID_destinationTransportPort, 0},
	{CN_proto, 0, IPFIX_TYPEID_protocolIdentifier, 0 },
	{CN_dstTos, 0, IPFIX_TYPEID_ipClassOfService, 0},
	{CN_bytes, 0, IPFIX_TYPEID_octetDeltaCount, 0},
	{CN_pkts, 0, IPFIX_TYPEID_packetDeltaCount, 0},
	{CN_firstSwitched, 0, IPFIX_TYPEID_flowStartSeconds, 0}, // default value is invalid/not used for this ent
	{CN_lastSwitched, 0, IPFIX_TYPEID_flowEndSeconds, 0}, // default value is invalid/not used for this entry
	{CN_firstSwitchedMillis, 0, IPFIX_TYPEID_flowStartMilliseconds, 0},
	{CN_lastSwitchedMillis, 0, IPFIX_TYPEID_flowEndMilliseconds, 0},
	{CN_tcpControlBits, 0, IPFIX_TYPEID_tcpControlBits, 0},
	//TODO: use enterprise number for the following extended types (Gerhard, 12/2009)
	{CN_revbytes, 0, IPFIX_TYPEID_octetDeltaCount, IPFIX_PEN_reverse},
	{CN_revpkts, 0, IPFIX_TYPEID_packetDeltaCount, IPFIX_PEN_reverse},
	{CN_revFirstSwitched, 0, IPFIX_TYPEID_flowStartSeconds, IPFIX_PEN_reverse}, 
	// default value is invalid/not used for this entry
	{CN_revLastSwitched, 0, IPFIX_TYPEID_flowEndSeconds, IPFIX_PEN_reverse}, 
	// default value is invalid/not used for this entry
	{CN_revFirstSwitchedMillis, 0, IPFIX_TYPEID_flowStartMilliseconds, IPFIX_PEN_reverse},
	{CN_revLastSwitchedMillis, 0, IPFIX_TYPEID_flowEndMilliseconds, IPFIX_PEN_reverse},
	{CN_revTcpControlBits, 0, IPFIX_TYPEID_tcpControlBits, IPFIX_PEN_reverse},
	{CN_maxPacketGap, 0, IPFIX_ETYPEID_maxPacketGap, IPFIX_PEN_vermont|IPFIX_PEN_reverse},
	{CN_exporterID, 0, EXPORTERID, 0},
	{0} // last entry must be 0
};

/**
 * Compare two source IDs and check if exporter is the same (i.e., same IP address and observationDomainId)
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
	if (con.isFailed()) return 0;
  
  // Connect
  string err;
  mongo::HostAndPort dbLogon;
  dbLogon = mongo::HostAndPort(dbHost, dbPort);
  msg(MSG_INFO,"IpfixDbWriterMongo: Connection details: %s", dbLogon.toString().c_str());
  if(!con.connect(dbLogon, err))
	{
		msg(MSG_FATAL,"IpfixDbWriterMongo: Mongo connect failed. Error: %s", err.c_str());
		return 1;
	}
    
  if(!dbUser.empty() && !dbPassword.empty()) 
  {
    // we need to authenticate
    if(!con.auth(dbName, dbUser, dbPassword, err))
	  {
		  msg(MSG_FATAL,"IpfixDbWriterMongo: Mongo authentication failed. Error: %s", err.c_str());
		  return 1;
	  }
  }

  // create couter to support incrementing Exporter IDs
  if(con.findOne(dbCollectionCounters, QUERY("_id" << "exporterCounter")).isEmpty())
  {
    mongo::BSONObjBuilder b;
    b << "_id" << "exporterCounter" << "c" << 0;
    mongo::BSONObj obj = b.obj();
    con.insert(dbCollectionCounters, obj);
  }

	msg(MSG_DEBUG,"IpfixDbWriterMongo: Mongo connection successful");
	dbError = false;
	return 0;
}

/**
 * save record to database
 */
void IpfixDbWriterMongo::processDataDataRecord(const IpfixRecord::SourceID& sourceID,
		TemplateInfo& dataTemplateInfo, uint16_t length,
		IpfixRecord::Data* data)
{
  mongo::BSONObj obj;
	msg(MSG_DEBUG, "IpfixDbWriter: Processing data record");

	if (dbError) {
		msg(MSG_DEBUG, "IpfixDbWriter: reconnecting to DB");
		connectToDB();
		if (dbError) return;
	}

	/* get new insert */
	if(srcId.observationDomainId != 0) {
		// use default source id
		obj = getInsertObj(srcId, dataTemplateInfo, length, data);
	} else {
		obj = getInsertObj(sourceID, dataTemplateInfo, length, data);
	}


	// start new insert statement if necessary
	if (numberOfInserts == 0) {
		// start insert statement
		bufferedObjects.clear();
    bufferedObjects.push_back(obj);
		numberOfInserts = 1;
	} else {
		// append object
    bufferedObjects.push_back(obj);
		numberOfInserts++;
	}

	// write to db if maxInserts is reached
	if(numberOfInserts == maxInserts) {
		msg(MSG_DEBUG, "IpfixDbWriter: Writing buffered records to database");
		writeToDb();
		numberOfInserts = 0;
	}
}

/**
 *	loop over properties and template to get the IPFIX values in correct order to store in database
 *	The result is written to BSON Object, and flowstart is returned
 */
mongo::BSONObj IpfixDbWriterMongo::getInsertObj(const IpfixRecord::SourceID& sourceID,
		TemplateInfo& dataTemplateInfo,uint16_t length, IpfixRecord::Data* data)
{
	uint64_t intdata = 0;
	uint64_t intdata2 = 0;
	uint32_t k;
	bool notfound, notfound2;
	mongo::BSONObjBuilder obj;

	time_t flowstartsec = 0;

	if (!allProp) {
	/** loop over a subset of elements (selected properties) and loop over the IPFIX_TYPEID of the record
	 *  to get the corresponding data to store and make insert statement
	 */
		for(vector<Property>::iterator prop = documentProperties.begin(); prop != documentProperties.end(); prop++) {
			if (prop->ipfixId == EXPORTERID) {
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
						if( dataTemplateInfo.fieldInfo[k].type.enterprise ==  prop->enterprise &&
							dataTemplateInfo.fieldInfo[k].type.id == prop->ipfixId) {
							notfound = false;
							intdata = getData(dataTemplateInfo.fieldInfo[k].type,(data+dataTemplateInfo.fieldInfo[k].offset));
							DPRINTF("IpfixDbWriterMongo::getData: really saw ipfix id %d in packet with intdata %llX, type %d, length %d and offset %X",
							  prop->ipfixId, intdata, dataTemplateInfo.fieldInfo[k].type.id, dataTemplateInfo.fieldInfo[k].type.length,
							  dataTemplateInfo.fieldInfo[k].offset);
							break;
						}
					}
				}
				if( dataTemplateInfo.dataCount > 0 && notfound) {
					// look in static data fields of template for data
					for(k=0; k < dataTemplateInfo.dataCount; k++) {
						if(dataTemplateInfo.fieldInfo[k].type.enterprise == prop->enterprise && dataTemplateInfo.dataInfo[k].type.id == prop->ipfixId) {
							notfound = false;
							intdata = getData(dataTemplateInfo.dataInfo[k].type,(dataTemplateInfo.data+dataTemplateInfo.dataInfo[k].offset));
							break;
						}
					}
				}
				if(notfound) {
					notfound2 = true;
					// for some Ids, we have an alternative
					if(prop->enterprise == 0) {
						switch (prop->ipfixId) {
							case IPFIX_TYPEID_flowStartSeconds:
								if(dataTemplateInfo.fieldCount > 0) {
									for(k=0; k < dataTemplateInfo.fieldCount; k++) {
										// look for alternative (flowStartMilliseconds/1000)
										if(dataTemplateInfo.fieldInfo[k].type.id == IPFIX_TYPEID_flowStartMilliseconds) {
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
										// look for alternative (flowEndMilliseconds/1000)
										if(dataTemplateInfo.fieldInfo[k].type.id == IPFIX_TYPEID_flowEndMilliseconds) {
											intdata = getData(dataTemplateInfo.fieldInfo[k].type,(data+dataTemplateInfo.fieldInfo[k].offset)) / 1000;
											notfound = false;
											break;
										}
										// if no flow end time is available, maybe this is from a netflow from Cisco
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
					} else if (prop->enterprise==IPFIX_PEN_reverse) {
						switch (prop->ipfixId) {
							case IPFIX_TYPEID_flowStartSeconds:
								// look for alternative (revFlowStartMilliseconds/1000)
								if(dataTemplateInfo.fieldCount > 0) {
									for(k=0; k < dataTemplateInfo.fieldCount; k++) {
										if(dataTemplateInfo.fieldInfo[k].type == InformationElement::IeInfo(IPFIX_TYPEID_flowStartMilliseconds, IPFIX_PEN_reverse)) {
											intdata = getData(dataTemplateInfo.fieldInfo[k].type,(data+dataTemplateInfo.fieldInfo[k].offset)) / 1000;
											notfound = false;
											break;
										}
									}
								}
								break;
							case IPFIX_TYPEID_flowEndSeconds:
								// look for alternative (revFlowEndMilliseconds/1000)
								if(dataTemplateInfo.fieldCount > 0) {
									for(k=0; k < dataTemplateInfo.fieldCount; k++) {
										if(dataTemplateInfo.fieldInfo[k].type == InformationElement::IeInfo(IPFIX_TYPEID_flowEndMilliseconds, IPFIX_PEN_reverse)) {
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
						intdata = prop->defaultValue;
				}

				// we need extra treatment for timing related fields
				if(prop->enterprise == 0 ) {
					switch (prop->ipfixId) {
						case IPFIX_TYPEID_flowStartSeconds:
							// save time for table access
							if (flowstartsec==0) flowstartsec = intdata;
							break;

						case IPFIX_TYPEID_flowEndSeconds:
							break;

						case IPFIX_TYPEID_flowStartMilliseconds:
							// if flowStartSeconds is not stored in one of the columns, but flowStartMilliseconds is,
							// then we use flowStartMilliseconds for table access
							// This is realized by storing this value only if flowStartSeconds has not yet been seen.
							// A later appearing flowStartSeconds will override this value.
							if (flowstartsec==0)
								flowstartsec = intdata/1000;
						case IPFIX_TYPEID_flowEndMilliseconds:
							// in the database the millisecond entry is counted from last second
							intdata %= 1000;
							break;
					}
				} else if (prop->enterprise==IPFIX_PEN_reverse)
					switch (prop->ipfixId) {
						case IPFIX_TYPEID_flowStartMilliseconds:
						case IPFIX_TYPEID_flowEndMilliseconds:
							// in the database the millisecond entry is counted from last second
							intdata %= 1000;
							break;
					}
			}
		msg(MSG_DEBUG, "saw ipfix id %s (element ID %d) in packet with intdata %llX", prop->propertyName,
							prop->ipfixId, static_cast<int64_t>(intdata));
					if (beautyProp)
						obj << prop->propertyName << static_cast<long long int>(intdata);
					else
						obj << boost::lexical_cast<std::string>(prop->ipfixId).c_str() << static_cast<long long int>(intdata);
						
					if (flowstartsec == 0) {
						msg(MSG_ERROR, "IpfixDbWriterMongo: Failed to get timing data from record. Will be saved in default table.");
					}
			}
		} else {
			/* Dump all elements to DB */
			if(dataTemplateInfo.fieldCount > 0) {
				// look in ipfix records
				for(int k=0; k < dataTemplateInfo.fieldCount; k++) {
						intdata = getData(dataTemplateInfo.fieldInfo[k].type,(data+dataTemplateInfo.fieldInfo[k].offset));
						DPRINTF("IpfixDbWriterMongo::getData: dumping from packet intdata %llX, type %d, length %d and offset %X",
						  intdata, dataTemplateInfo.fieldInfo[k].type.id, dataTemplateInfo.fieldInfo[k].type.length,
						  dataTemplateInfo.fieldInfo[k].offset);
						obj << boost::lexical_cast<std::string>(dataTemplateInfo.fieldInfo[k].type.id).c_str() << static_cast<long long int>(intdata);
					}
				}
			
			if( dataTemplateInfo.dataCount > 0) {
				// look in static data fields of template for data
				for(int k=0; k < dataTemplateInfo.dataCount; k++) {
						intdata = getData(dataTemplateInfo.dataInfo[k].type,(dataTemplateInfo.data+dataTemplateInfo.dataInfo[k].offset));
						obj << boost::lexical_cast<std::string>(dataTemplateInfo.fieldInfo[k].type.id).c_str() << static_cast<long long int>(intdata);
					}
				}
		}
	return obj.obj();
}


/*
 * Write Objects to database
 */
int IpfixDbWriterMongo::writeToDb()
{
  con.insert(dbCollectionFlows, bufferedObjects);
  if(con.getLastError() != ""){
		msg(MSG_FATAL, "IpfixDbWriterMongo: Failed to write to DB.");
    return 1;
  }
	return 0; 
}

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
  mongo::BSONObj exporter = con.findOne(dbCollectionExporters, QUERY("sourceID" << sourceID.observationDomainId << "srcIp" << expIp));
	// search exporter collection
  if(exporter.isEmpty()){
    mongo::BSONObj exporterCounter;
    mongo::BSONObj cmd;
    cmd = BSON( "findAndModify" << "counters" << "query" << BSON("_id" << "exporterCounter") << "update" << BSON("$inc" << BSON("c" << 1)));
    msg(MSG_DEBUG, "FIND AND MODIFY: %s", cmd.toString().c_str());
    con.runCommand(dbName, cmd, exporterCounter);
    mongo::BSONObjBuilder b;
    id = exporterCounter.getObjectField("value").getIntField("c");
    b << "sourceID" << sourceID.observationDomainId << "srcIP" << expIp << "id" <<  id;
    mongo::BSONObj obj = b.obj();
    con.insert(dbCollectionExporters, obj);
  } else {
    id = exporter.getIntField("id");
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
	// TODO: workout a proper modular interpreter
	if (type.id >= IPFIX_TYPEID_mplsTopLabelStackSection &&
	    type.id <= IPFIX_TYPEID_mplsLabelStackSection10) {
			// Properly decode MPLS Label, Experimental and S fields
			// TODO: Provide all separated fields
			uint32_t MPLSStruct = 0;
			uint32_t Label = 0, Exp = 0, S = 0;
			memcpy(&MPLSStruct, data, 3);
			Label = MPLSStruct & 0xfffff0;
			Exp = MPLSStruct & 0x00000e;
			S = MPLSStruct & 0x000001;
			return ntohl(Label);
		}
	
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
			// TODO: dynamic lenght octectArray fields
			printf("Got element %d with unparsable length(%d).\n", type.id, type.length);
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
		unsigned port, uint32_t observationDomainId, uint16_t maxStatements,
		const vector<string>& propertyNames, bool beautifyProperties, bool allProperties)
	: currentExporter(NULL), numberOfInserts(0), maxInserts(maxStatements),
	dbHost(hostname), dbName(database), dbUser(username), dbPassword(password), dbPort(port), con(0),
	beautyProp(beautifyProperties), allProp(allProperties)
{
	int i;

	// set default source id
	srcId.exporterAddress.len = 0;
	srcId.observationDomainId = observationDomainId;
	srcId.exporterPort = 0;
	srcId.receiverPort = 0;
	srcId.protocol = 0;
	srcId.fileDescriptor = 0;

  // Set Values for Collections
  dbCollectionFlows.append(dbName).append(".flows");
  dbCollectionExporters.append(dbName).append(".exporters");
  dbCollectionCounters.append(dbName).append(".counters");

	/* get properties */
	for(vector<string>::const_iterator prop = propertyNames.begin(); prop != propertyNames.end(); prop++) {
		i = 0;
		while(identify[i].propertyName != 0) {
			if(prop->compare(identify[i].propertyName) == 0) {
				Property p = identify[i];
				documentProperties.push_back(p);
				break;
			}
			i++;
		}
	}
	
  if(propertyNames.empty() && ! allProp)
		THROWEXCEPTION("IpfixDbWriterMongo: cannot initiate with no properties");

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
