/*
 * IPFIX Database Reader for Oracle DBs
 * Copyright (C) 2006 Jürgen Abberger
 * Copyright (C) 2006-2012 Lothar Braun
 * Copyright (C) 2008 Gerhard Muenz
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

#ifdef ORACLE_SUPPORT_ENABLED

#include <stdexcept>
#include <string.h>
#include <stdlib.h>
#include <sstream>
#include "IpfixDbReaderOracle.hpp"
#include "IpfixDbCommon.hpp"
#include "common/msg.h"


InstanceManager<IpfixTemplateRecord> IpfixDbReaderOracle::templateRecordIM("DbReaderIpfixTemplateRecord", 1);
InstanceManager<IpfixDataRecord> IpfixDbReaderOracle::dataRecordIM("DbReaderIpfixDataRecord", 50);
InstanceManager<IpfixTemplateDestructionRecord> IpfixDbReaderOracle::templateDestructionRecordIM("DbReaderIpfixTemplateDestructionRecord", 1);

/***** Internal Functions ****************************************************/

void copyUintNetByteOrder(IpfixRecord::Data* dest, char* src, InformationElement::IeInfo type);

/**
 * First send a a new template, then send the dataTemplates for all stored
 * tables.
 */
void* IpfixDbReaderOracle::readFromDB(void* ipfixDbReader_)
{
	IpfixDbReaderOracle* ipfixDbReader = (IpfixDbReaderOracle*)ipfixDbReader_;

	ipfixDbReader->registerCurrentThread();

	msg(MSG_DIALOG, "IpfixDbReaderOracle: Start sending tables");
	for(vector<string>::iterator i = ipfixDbReader->tables.begin(); i != ipfixDbReader->tables.end() && !ipfixDbReader->exitFlag; i++) {
		// check if table can contain flows between firstFlowTime 
		// and lastFlowTime (in case these are set).
		// Only replay traffic from tables that potentially contain flows
		// from this time interval. If startTime == 0, all flows from the
		// beginning of the database will be read. If endTime == 0, all flows
		// until the end of the last table will be read.
		if (ipfixDbReader->firstFlowTime != 0 || ipfixDbReader->lastFlowTime) {
			if (!ipfixDbReader->isTableBetweenTimestamps(*i, ipfixDbReader->firstFlowTime, ipfixDbReader->lastFlowTime)) 
				continue;
		}

		boost::shared_ptr<TemplateInfo> templateInfo(new TemplateInfo);
		templateInfo->setId = TemplateInfo::IpfixTemplate;
		if(ipfixDbReader->dbReaderSendNewTemplate(templateInfo, *i) != 0)
		{
		    msg(MSG_ERROR, "IpfixDbReaderOracle: Template error, skip table");
		    continue;
		}
		ipfixDbReader->dbReaderSendTable(templateInfo, *i);
		ipfixDbReader->dbReaderDestroyTemplate(templateInfo);

	}

	ipfixDbReader->unregisterCurrentThread();
	
	msg(MSG_DIALOG,"IpfixDbReaderOracle: Sending from database is done");
	return 0;
}
/**
 * Constructs a template from the table data and sends it to all connected
 * modules.
 */
int IpfixDbReaderOracle::dbReaderSendNewTemplate(boost::shared_ptr<TemplateInfo> templateInfo, const string& tableName)
{
	// reset record length 
	recordLength  = 0;

	/**get columnsname of the table*/
	if(getColumns(tableName) != 0) {
		msg(MSG_ERROR,"IpfixDbReaderOracle: Could not get columns for template");
		return 1;
	}

	for(vector<columnDB>::iterator i = columns.begin(); i != columns.end(); i++) {			
		templateInfo->fieldCount++;
		templateInfo->fieldInfo = (TemplateInfo::FieldInfo*)realloc(templateInfo->fieldInfo,
						      sizeof(TemplateInfo::FieldInfo)*templateInfo->fieldCount);
		TemplateInfo::FieldInfo* fi = &templateInfo->fieldInfo[templateInfo->fieldCount - 1];	
		fi->type.id = i->ipfixId;
		fi->type.length = i->length;
		fi->type.enterprise = 0;
		fi->offset = recordLength;
		recordLength = recordLength + fi->type.length; 
	}

	/* Pass Data Template to flowSinks */
	IpfixTemplateRecord* ipfixRecord = templateRecordIM.getNewInstance();
	ipfixRecord->sourceID = srcId;
	ipfixRecord->templateInfo = templateInfo;
	send(ipfixRecord);
	msg(MSG_DEBUG,"IpfixDbReaderOracle: sent template for table %s", tableName.c_str());
	return 0;
}


void copyUintNetByteOrder(IpfixRecord::Data* dest, char* src, InformationElement::IeInfo type) {
        switch (type.length) {
        case 1:
		*(uint8_t*)dest = *(uint8_t*)src;
                return;
        case 2:
		*(uint16_t*)dest = htons(*(uint16_t*)src);
                return;
        case 4:
		*(uint32_t*)dest = htonl(*(uint32_t*)src);
                return;
        case 8:
		*(uint64_t*)dest = htonll(*(uint64_t*)src);
                return;
        default:
                msg(MSG_ERROR, "IpfixDbReaderOracle: Uint with length %d unparseable", type.length);
                return;
        }
}


/**
 * Select a given table and get the values by reading
 * the database. The Typs of the values from database are
 * strings, therefore they must change into IPFIX format 
*/

int IpfixDbReaderOracle::dbReaderSendTable(boost::shared_ptr<TemplateInfo> templateInfo, const string& tableName)
{
	std::ostringstream sql;
	oracle::occi::Statement *stmt = NULL;
	oracle::occi::ResultSet *rs = NULL;

	unsigned offset = 0;
	uint64_t delta = 0;		// 64 bit to avoid castings in the case of flowStartMilliseconds
	uint32_t flowTime = 0;		// in seconds, so 32 bit are sufficient
	uint32_t lastFlowTime = 0;
	uint64_t tmp;
	bool first = true; 
	unsigned j = 0;

	msg(MSG_VDEBUG, "IpfixDbReaderOracle: Sending table %s", tableName.c_str());
	
	sql << "SELECT " << columnNames << " FROM "<< tableName;

	if (firstFlowTime || lastFlowTime) {
		sql << whereClause;
	}

	// at full speed, we do not make time shifts or reorder
	if(fullspeed)
		timeshift = false; // timeshift disabled in fullspeed mode
	else
		sql << orderBy;

	// create the oracle statement
	try {
		stmt = con->createStatement(sql.str());
	} catch (oracle::occi::SQLException& ex) {
		msg(MSG_FATAL, "IpfixDbReaderOracle: Error creating statement: %s", ex.getMessage().c_str());
		return 1;
	}
	
	//msg(MSG_VDEBUG, "IpfixDbReaderOracle: SQL query: %s", query.c_str());
	try {
		stmt->setPrefetchRowCount(1);
		rs = stmt->executeQuery();
	} catch (oracle::occi::SQLException& ex) {
		msg(MSG_FATAL,"IpfixDbWriterOracle: Error executing statement: %s", ex.getMessage().c_str());
		con->terminateStatement(stmt);
	}

	if (!rs) {
		msg(MSG_ERROR, "IpfixDbWriterOracle: Table %s was empty!", tableName.c_str());
		return 1;
	}

	msg(MSG_INFO,"IpfixDbReaderOracle:  Start sending records from table %s", tableName.c_str());

	try {
	while((rs->next()) && !exitFlag) {
		if (first) {
			j = 1;
			for(vector<columnDB>::iterator i = columns.begin(); i != columns.end(); ++i) {
				if(i->ipfixId == IPFIX_TYPEID_flowEndSeconds) {
					delta = time(NULL) - atoll(rs->getString(j).c_str());
					lastFlowTime = atoll(rs->getString(j).c_str()) + delta;
					first = false; 
					break; 
				} else if(i->ipfixId == IPFIX_TYPEID_flowEndMilliSeconds) {
					delta = time(NULL) - atoll(rs->getString(j).c_str())/1000;
					lastFlowTime = atoll(rs->getString(j).c_str())/1000 + delta;
					first = false; 
					break; 
				}
				j++;
			}
			if (first) {
				msg(MSG_ERROR, "IpfixDbReaderOracle: no flowEndSeconds or floweEndMilliSeconds column in table %s", tableName.c_str());
				stmt->closeResultSet(rs);
				con->terminateStatement(stmt);
				return 1;
			}
			if (timeshift)
				msg(MSG_DEBUG, "IpfixDbReaderOracle: time shift is %d seconds", delta);
		}
		// build new record
		boost::shared_array<IpfixRecord::Data> data(new IpfixRecord::Data[recordLength]);
		offset = 0;
		j = 0;
		for(vector<columnDB>::iterator i = columns.begin(); i != columns.end(); ++i) {
			switch(i->ipfixId) {
			case IPFIX_TYPEID_flowEndSeconds:
			        flowTime = atoll(rs->getString(j + 1).c_str()) + delta;
			case IPFIX_TYPEID_flowStartSeconds:
				tmp = atoll(rs->getString(j + 1).c_str());
				// do time shift if required
				if(timeshift)
					tmp += delta;
				copyUintNetByteOrder(data.get() + templateInfo->fieldInfo[j].offset,
						     (char*)&tmp,
						     templateInfo->fieldInfo[j].type);
				offset += templateInfo->fieldInfo[j].type.length;
				break;
			case IPFIX_TYPEID_flowEndMilliSeconds:
			        flowTime = atoll(rs->getString(j + 1).c_str())/1000 + delta;
			case IPFIX_TYPEID_flowStartMilliSeconds:
				tmp = atoll(rs->getString(j + 1).c_str());
				// do time shift if required
				if(timeshift)
					tmp += 1000*delta;
				copyUintNetByteOrder(data.get() + templateInfo->fieldInfo[j].offset,
						     (char*)&tmp,
						     templateInfo->fieldInfo[j].type);
				offset += templateInfo->fieldInfo[j].type.length;
				break;
			case IPFIX_TYPEID_octetDeltaCount:
			case IPFIX_TYPEID_packetDeltaCount:
			case IPFIX_TYPEID_destinationIPv4Address:
			case IPFIX_TYPEID_sourceIPv4Address:
			case IPFIX_TYPEID_sourceTransportPort:
			case IPFIX_TYPEID_destinationTransportPort:
			case IPFIX_TYPEID_protocolIdentifier:
			case IPFIX_TYPEID_classOfServiceIPv4:
				tmp = atoll(rs->getString(j + 1).c_str());
				copyUintNetByteOrder(data.get() + templateInfo->fieldInfo[j].offset,
						     (char*)&tmp,
						     templateInfo->fieldInfo[j].type);
				offset += templateInfo->fieldInfo[j].type.length;
				break;
			}
			j++;
		}
	
		/** according to flowstarttime wait for sending the record*/
		if(!fullspeed && (flowTime != lastFlowTime)) {
			time_t t = time(NULL);
			if (t > (int)flowTime) {
				msg(MSG_ERROR, "IpfixDbReaderOracle: Sending flows too slowly");
			} else {
				sleep (flowTime - t);
			}
			lastFlowTime = flowTime;
		}



		IpfixDataRecord* ipfixRecord = dataRecordIM.getNewInstance();
		ipfixRecord->sourceID = srcId;
		ipfixRecord->templateInfo = templateInfo;
		ipfixRecord->dataLength = offset; // = recordLength
		ipfixRecord->message = data;
		ipfixRecord->data = data.get();
		send(ipfixRecord);
	}
	stmt->closeResultSet(rs);
	con->terminateStatement(stmt);
	
	} catch (oracle::occi::SQLException& ex) {
		msg(MSG_ERROR, "Caught generic SQL exception");
		return 1;
	}

	if(!exitFlag)
		msg(MSG_INFO,"IpfixDbReaderOracle: Sending from table %s done", tableName.c_str());
	else
		msg(MSG_INFO,"IpfixDbReaderOracle: Sending from table %s aborted", tableName.c_str());

	return 0;
}


int IpfixDbReaderOracle::dbReaderDestroyTemplate(boost::shared_ptr<TemplateInfo> templateInfo)
{
	IpfixTemplateDestructionRecord* ipfixRecord = templateDestructionRecordIM.getNewInstance();
	ipfixRecord->sourceID = srcId;
	ipfixRecord->templateInfo = templateInfo;
	send(ipfixRecord);
	msg(MSG_DEBUG,"IpfixDbReaderOracle: Template destroyed");

	return 0;
}


/**
 * get all tables in database that matches with the wildcard "h\_%"
 **/
int IpfixDbReaderOracle::getTables()
{
	std::ostringstream sql;
        oracle::occi::Statement *stmt = NULL;
        oracle::occi::ResultSet *rs = NULL;


        sql << "SELECT table_name FROM user_tables WHERE table_name LIKE 'H_%' ORDER BY table_name ASC";
        // create the oracle statement
        try {
                stmt = con->createStatement(sql.str());
        } catch (oracle::occi::SQLException& ex) {
                msg(MSG_FATAL, "IpfixDbReaderOracle: Error creating statement: %s", ex.getMessage().c_str());
                return 1;
        }

        //msg(MSG_VDEBUG, "IpfixDbReaderOracle: SQL query: %s", query.c_str());
        try {
                stmt->setPrefetchRowCount(1);
                rs = stmt->executeQuery();
        } catch (oracle::occi::SQLException& ex) {
                msg(MSG_FATAL,"IpfixDbWriterOracle: Error executing statement: %s", ex.getMessage().c_str());
                con->terminateStatement(stmt);
        }

        if (!rs) {
                msg(MSG_ERROR, "IpfixDbWriterOracle: Found no flow tables!");
                return 1;
        }

	try {
        while((rs->next()) && !exitFlag) {
		tables.push_back(rs->getString(1));
		msg(MSG_VDEBUG, "IpfixDbReaderOracle: table %s", tables.back().c_str());
	}

        stmt->closeResultSet(rs);
        con->terminateStatement(stmt);

        } catch (oracle::occi::SQLException& ex) {
                msg(MSG_ERROR, "Caught generic SQL exception");
                return 1;
        }
	
	return 0;
}


/**
 * Get the names of columns
 */
int IpfixDbReaderOracle::getColumns(const string& tableName)
{
        oracle::occi::Statement *stmt = NULL;
        oracle::occi::ResultSet *rs = NULL;

	string query = "SELECT column_name FROM cols WHERE table_name = '" + tableName + "'";
        // create the oracle statement
        try {
                stmt = con->createStatement(query);
        } catch (oracle::occi::SQLException& ex) {
                msg(MSG_FATAL, "IpfixDbReaderOracle: Error creating statement: %s", ex.getMessage().c_str());
                return 1;
        }

        //msg(MSG_VDEBUG, "IpfixDbReaderOracle: SQL query: %s", query.c_str());
        try {
                stmt->setPrefetchRowCount(1);
                rs = stmt->executeQuery();
        } catch (oracle::occi::SQLException& ex) {
                msg(MSG_FATAL,"IpfixDbWriterOracle: Error executing statement: %s", ex.getMessage().c_str());
                con->terminateStatement(stmt);
        }

        if (!rs) {
                msg(MSG_ERROR, "IpfixDbWriterOracle: Flow tables do not have columns??");
                return 1;
        }

	columns.clear();
	columnNames = "";
	orderBy = ""; 
	whereClause = "";
	bool haveFirstMillis = false;
	bool haveLastMillis = false;

	try {
        while((rs->next()) && !exitFlag) {
		bool found = true;
		if(strcasecmp(rs->getString(1).c_str(), CN_dstIP) == 0) {
			columnNames = columnNames + ", " + CN_dstIP;
			columnDB tmp = {IPFIX_TYPEID_destinationIPv4Address, IPFIX_LENGTH_destinationIPv4Address};
			columns.push_back(tmp);
		} else if(strcasecmp(rs->getString(1).c_str(), CN_srcIP) == 0) {
			columnNames = columnNames + ", " + CN_srcIP;
			columnDB tmp = {IPFIX_TYPEID_sourceIPv4Address, IPFIX_LENGTH_sourceIPv4Address};
			columns.push_back(tmp);
		} else if(strcasecmp(rs->getString(1).c_str(), CN_dstPort) == 0) {
			columnNames = columnNames + ", " + CN_dstPort;
			columnDB tmp = {IPFIX_TYPEID_destinationTransportPort, IPFIX_LENGTH_destinationTransportPort};
			columns.push_back(tmp);
		} else if(strcasecmp(rs->getString(1).c_str(), CN_srcPort) == 0) {
			columnNames = columnNames + ", " + CN_srcPort;
			columnDB tmp = {IPFIX_TYPEID_sourceTransportPort, IPFIX_LENGTH_sourceTransportPort};
			columns.push_back(tmp);
		} else if(strcasecmp(rs->getString(1).c_str(), CN_proto) == 0) {
			columnNames = columnNames + ", " + CN_proto;
			columnDB tmp = {IPFIX_TYPEID_protocolIdentifier, IPFIX_LENGTH_protocolIdentifier};
			columns.push_back(tmp);
		} else if(strcasecmp(rs->getString(1).c_str(), CN_dstTos) == 0) {
			columnNames = columnNames + ", " + CN_dstTos;
			columnDB tmp = {IPFIX_TYPEID_classOfServiceIPv4, IPFIX_LENGTH_classOfServiceIPv4};
			columns.push_back(tmp);
		} else if(strcasecmp(rs->getString(1).c_str(), CN_bytes) == 0) {
			columnNames = columnNames + ", " + CN_bytes;
			columnDB tmp = {IPFIX_TYPEID_octetDeltaCount, IPFIX_LENGTH_octetDeltaCount};
			columns.push_back(tmp);
		} else if(strcasecmp(rs->getString(1).c_str(), CN_pkts) == 0) {
			columnNames = columnNames + ", " + CN_pkts;
			columnDB tmp = {IPFIX_TYPEID_packetDeltaCount, IPFIX_LENGTH_packetDeltaCount};
			columns.push_back(tmp);
		} else if(strcasecmp(rs->getString(1).c_str(), CN_firstSwitched) == 0) {
			columnNames = columnNames + ", " + CN_firstSwitched;
			columnDB tmp = {IPFIX_TYPEID_flowStartSeconds, IPFIX_LENGTH_flowStartSeconds};
			columns.push_back(tmp);
			std::stringstream out;
			out << " WHERE " << CN_firstSwitched << " >= " << firstFlowTime;
			if (lastFlowTime) {
				out << " AND " << CN_firstSwitched << " <= " << lastFlowTime;
			}
			whereClause = out.str();
		} else if(strcasecmp(rs->getString(1).c_str(), CN_lastSwitched) == 0) {
			columnNames = columnNames + ", " + CN_lastSwitched;
			columnDB tmp = {IPFIX_TYPEID_flowEndSeconds, IPFIX_LENGTH_flowEndSeconds};
			columns.push_back(tmp);
			orderBy = " ORDER BY ";
			orderBy.append(CN_lastSwitched);
		} else if(strcasecmp(rs->getString(1).c_str(), CN_firstSwitchedMillis) == 0) {
			haveFirstMillis = true;
		} else if(strcasecmp(rs->getString(1).c_str(), CN_lastSwitchedMillis) == 0) {
			haveLastMillis = true;
		} else if(strcasecmp(rs->getString(1).c_str(), CN_exporterID) != 0) { 
			msg(MSG_INFO, "IpfixDbReaderOracle: Unsupported column: %s", rs->getString(1).c_str());
			found = false;
		}
		if(found)
			msg(MSG_VDEBUG, "IpfixDbReaderOracle: column %s (%d)", rs->getString(1).c_str(), columns.back().ipfixId);
	}
	
	// if we have found seconds and milliseconds, forge the columns to get flowStart/EndMilliseconds
	string::size_type pos;
	if(haveFirstMillis && (pos = columnNames.find(CN_firstSwitched, 0)) != string::npos) {
		columnNames.insert(pos, "+1000*");
		columnNames.insert(pos, CN_firstSwitchedMillis);
		for(vector<columnDB>::iterator i = columns.begin(); i != columns.end(); i++) {
			if(i->ipfixId == IPFIX_TYPEID_flowStartSeconds) {
				i->ipfixId = IPFIX_TYPEID_flowStartMilliSeconds;
				i->length = IPFIX_LENGTH_flowStartMilliSeconds;
				break;
			}
		}
	}
	if(haveLastMillis && (pos = columnNames.find(CN_lastSwitched, 0)) != string::npos) {
		columnNames.insert(pos, "+1000*");
		columnNames.insert(pos, CN_lastSwitchedMillis);
		orderBy = " ORDER BY ";
		orderBy.append(CN_lastSwitchedMillis);
		orderBy.append("+1000*");
		orderBy.append(CN_lastSwitched);
		for(vector<columnDB>::iterator i = columns.begin(); i != columns.end(); i++) {
			if(i->ipfixId == IPFIX_TYPEID_flowEndSeconds) {
				i->ipfixId = IPFIX_TYPEID_flowEndMilliSeconds;
				i->length = IPFIX_LENGTH_flowEndMilliSeconds;
				break;
			}
		}
	}


	if(columnNames != "")
		columnNames.erase(0,2);

        stmt->closeResultSet(rs);
        con->terminateStatement(stmt);
        } catch (oracle::occi::SQLException& ex) {
                msg(MSG_ERROR, "Caught generic SQL exception");
                return 1;
        }

	return 0;
}


int IpfixDbReaderOracle::connectToDb(
		const string& hostName, const string& dbName, 
		const string& userName, const string& password,
		unsigned int port)
{
	dbError = true;

	// try to close connection in case we still have one
	// from a previous try
	if (con) {
		env->terminateConnection(con);
	}	

	msg(MSG_DEBUG, "IpfixDbReaderOracle: Creating environment.");
	try {
		env = oracle::occi::Environment::createEnvironment(oracle::occi::Environment::DEFAULT);
	} catch (oracle::occi::SQLException& ex) {
		msg(MSG_FATAL, "IpfixDbReaderOracle: Error while creating environment: %s.", ex.getMessage().c_str());
		msg(MSG_FATAL, "IpfixDbReaderOracle: Did you configure your Oracle environment?");
		return -1;
	}
	msg(MSG_DEBUG, "IpfixDbReaderOracle: Trying to connect to database ...");
	try 
	{
		char dbLogon[256];
		sprintf(dbLogon, "%s:%u/%s", hostName.c_str(), port, dbName.c_str());
		con = env->createConnection(userName, password, dbLogon);
	} catch (oracle::occi::SQLException& ex) 
	{
		msg(MSG_FATAL,"IpfixDbReaderOracle: Oracle connect failed. Error: %s", ex.getMessage().c_str());
		return 1;
	}
	msg(MSG_DEBUG,"IpfixDbReaderOracle: Successfully connected to Oracle DB");

	return 0;
}

/***** Exported Functions ****************************************************/

/**
 * Starts or resumes database
 * @param ipfixDbReaderOracle handle obtained by calling @c createipfixDbReader()
 */
void IpfixDbReaderOracle::performStart() 
{
	thread.run(this);
}

/**
 * Temporarily pauses database
 * @param ipfixDbReader handle obtained by calling @c createipfixDbReader()
 */
void IpfixDbReaderOracle::performShutdown() 
{
	thread.join();
}

/**
 * Frees memory used by an ipfixDbReader
 * @param ipfixDbWriter handle obtained by calling @c createipfixDbReader()
 */
IpfixDbReaderOracle::~IpfixDbReaderOracle() {
	env->terminateConnection(con);
	oracle::occi::Environment::terminateEnvironment(env);
}

/**
 * Creates a new ipfixDbReader. Do not forget to call @c startipfixDbReader() to begin reading from Database
 * @return handle to use when calling @c destroyipfixDbRreader()
 */
IpfixDbReaderOracle::IpfixDbReaderOracle(const string& hostname, const string& dbname,
				const string& username, const string& password,
				unsigned port, uint16_t observationDomainId, 
					 bool timeshift, bool fullspeed, uint32_t startTime, uint32_t endTime)
	: timeshift(timeshift), fullspeed(fullspeed), firstFlowTime(startTime), lastFlowTime(endTime), thread(readFromDB)
{
	srcId.reset(new IpfixRecord::SourceID);
	srcId->observationDomainId = observationDomainId;
	srcId->exporterAddress.len = 0;
	srcId->exporterPort = 0;
	srcId->receiverPort = 0;
	srcId->protocol = 0;
	srcId->fileDescriptor = 0;

	if (fullspeed) {
		msg(MSG_DEBUG, "IpfixDbReaderOracle: found fullspeed in configuration. Pushing flows as fast as possible.");
	}

	if (connectToDb(hostname, dbname, username, password, port)) {
		THROWEXCEPTION("IpfixDbReaderOracle creation failed");
	}
	
	/** get tables of the database*/
	if(getTables() != 0) {
		msg(MSG_ERROR,"IpfixDbReaderOracle: Error in function getTables");
		THROWEXCEPTION("IpfixDbReaderOracle creation failed");
	}

	if(fullspeed && timeshift) 
		msg(MSG_DIALOG, "IpfixDbReaderOracle: timeshift configured, but disabled in fullspeed mode");
}

bool IpfixDbReaderOracle::isTableBetweenTimestamps(const string& tablename, uint32_t start, uint32_t end)
{
	uint32_t tableStartTime, tableEndTime;
	std::stringstream str(tablename);
	char tmp[5];
	uint32_t timeStamp;
	uint32_t time;
	struct tm tableTime;

	bzero(&tableTime, sizeof(struct tm));
	// the table string lookes like H_20120202_10_0 and always
	// matches a length of 15 bytes;
	if (tablename.size() != 15) {
		msg(MSG_DEBUG, "IpfixDbReaderOracle: Found a table name \"%s\" which does not match the expected table name size of 15 bytes.", tablename.c_str());
		return false;
	}
	// get H_
	str.get(tmp, 3);
	if (std::string(tmp) != "H_") {
		msg(MSG_ERROR, "IpfixDbReaderOracle: Found table that does not begin with static prefix \"H_\" but with prefix \"%s\": %s", tmp, tablename.c_str());
		return false;
	}

	// get year 
	str.get(tmp, 5);
	time = atoi(tmp);
	if (time < 1970) {
		msg(MSG_ERROR, "IpfixDbReaderOracle: Found a year < 1970 in \"%s\": %d", tablename.c_str(), time);
		return false;
	}
	tableTime.tm_year = time - 1900;

	// get month
	str.get(tmp, 3);
	time = atoi(tmp);
	if (time < 1 || time > 12) {
		msg(MSG_ERROR, "IpfixDbReaderOracle: Found a non-valid month in \"%s\": %d", tablename.c_str(), time);
		return false;
	}
	tableTime.tm_mon = time - 1;
	
	// get day
	str.get(tmp, 3);
	time = atoi(tmp);
	if (time < 1 || time > 31) {
		msg(MSG_ERROR, "IpfixDbReaderOracle: Found a non-valid day in \"%s\": %d", tablename.c_str(), time);
		return false;
	}
	tableTime.tm_mday = time;

	// get separamter "_"
	str.get(tmp, 2);
	if (std::string(tmp) != "_") {
		msg(MSG_ERROR, "IpfixDbReaderOracle: Found non-valid separator in \"%s\" after day: %s", tablename.c_str(), tmp);
		return false;
	}
	
	// get hour
	str.get(tmp, 3);
	time = atoi(tmp);
	if (time > 23) {
		msg(MSG_ERROR, "IpfixDbReaderOracle: Found non-valid hour in \"%s\": %d", tablename.c_str(), time);
		return false;
	}
	tableTime.tm_hour = time;

	// get separator "_"
	str.get(tmp, 2);
	if (std::string(tmp) != "_") {
		msg(MSG_ERROR, "IpfixDbReaderOracle: Found non-valid separator in \"%s\" after hour: %s", tablename.c_str(), tmp);
		return false;
	}

	// get half hour
	str.get(tmp, 3);
	time = atoi(tmp);
	if (time == 0) {
		// we are within the first half hour
		tableTime.tm_min = 0;
	} else if (time == 1) {
		tableTime.tm_min = 30;
		// we are in the second half hour
	} else {
		msg(MSG_ERROR, "IpfixDbReaderOracle: Found non-valid half hour indicator in \"%s\": %s", tablename.c_str(), tmp);
		return false;
	}

	tableStartTime = timegm(&tableTime);
	tableEndTime   = tableStartTime + 30*60;
	if (start) {
		if (start > tableEndTime)
			return false;
	} 
	if (end) {
		if (end < tableStartTime) 
			return false;
	}
	return true;
}

#endif
