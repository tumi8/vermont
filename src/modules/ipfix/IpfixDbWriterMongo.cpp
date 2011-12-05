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

IpfixDbWriterMongo::Column identify [] = {
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

//// TOBECONTINUED
/**
 * (re)connect to database
 */
int IpfixDbWriterMongo::connectToDB()
{
	dbError = true;
	
	// close (in the case that it was already connected)
	if (con) env->terminateConnection(con);

	/** get the initial environment and connect */
	env = oracle::occi::Environment::createEnvironment(oracle::occi::Environment::DEFAULT);
	try 
	{
		char dbLogon[128];
		sprintf(dbLogon, "%s:%u/", dbHost.c_str(), dbPort);
		con = env->createConnection(dbUser, dbPassword, dbLogon);
	} catch (oracle::occi::SQLException& ex) 
	{
		msg(MSG_FATAL,"IpfixDbWriterMongo: Oracle connect failed. Error: %s", ex.getMessage().c_str());
		return 1;
	}
	msg(MSG_DEBUG,"IpfixDbWriterMongo: Oracle connection successful");
	
	if (createExporterTable()!=0) return 1;
	
	dbError = false;
	
	return 0;
}

int IpfixDbWriterMongo::createExporterTable()
{
	// check if table exists
	ostringstream sql;
	oracle::occi::Statement *stmt = NULL;
	oracle::occi::ResultSet *rs = NULL;
	sql << "SELECT COUNT(table_name) FROM user_tables WHERE table_name='EXPORTER'";
	try
	{
		stmt = con->createStatement(sql.str());
	}
	catch (oracle::occi::SQLException& ex)
	{
		msg(MSG_FATAL,"IpfixDbWriterMongo: %s", ex.getMessage().c_str());	
		return 1;		
	}
	if (stmt)
	{
		try 
		{
			stmt->setPrefetchRowCount(1);
			rs = stmt->executeQuery();			
		}
		catch (oracle::occi::SQLException& ex)
		{
			msg(MSG_FATAL,"IpfixDbWriterMongo: %s", ex.getMessage().c_str());	
			con->terminateStatement(stmt);
			return 1;					
		}
		if (rs)
		{
			while(rs->next())
			{
				if (rs->getInt(1)!= 0) 
				{
					msg(MSG_DEBUG,"IpfixDbWriterMongo: exporter table does exist");
					stmt->closeResultSet(rs);
					con->terminateStatement(stmt);
					return 0;	
				}
			}
			stmt->closeResultSet(rs);
		}
		con->terminateStatement(stmt);
	}

	// create table
	sql.str("");	
	sql << "CREATE TABLE exporter ( id NUMERIC(10) NOT NULL, sourceID NUMERIC(10), srcIP NUMERIC(10), CONSTRAINT exporter_pk PRIMARY KEY (id) ) TABLESPACE " << dbName;
	try
	{
		stmt = con->createStatement(sql.str());
	}
	catch (oracle::occi::SQLException& ex)
	{
		msg(MSG_FATAL,"IpfixDbWriterMongo: %s", ex.getMessage().c_str());	
		return 1;		
	}
	if (stmt)
	{
		try 
		{
			stmt->setPrefetchRowCount(1);
			rs = stmt->executeQuery();			
		}
		catch (oracle::occi::SQLException& ex)
		{
			msg(MSG_FATAL,"IpfixDbWriterMongo: %s", ex.getMessage().c_str());	
			con->terminateStatement(stmt);
			return 1;					
		}
		msg(MSG_DEBUG,"IpfixDbWriterMongo: exporter table created");
		stmt->closeResultSet(rs);
		con->terminateStatement(stmt);
	}
	
	// create counter
	// clear vars for reuse
	sql.str("");	
	sql << "CREATE sequence counter_for_exporter increment BY 1 start WITH 1 cache 2";
	try
	{
		stmt = con->createStatement(sql.str());
	}
	catch (oracle::occi::SQLException& ex)
	{
		msg(MSG_FATAL,"IpfixDbWriterMongo: %s", ex.getMessage().c_str());	
		return 1;		
	}
	if (stmt)
	{
		try 
		{
			stmt->setPrefetchRowCount(1);
			rs = stmt->executeQuery();			
		}
		catch (oracle::occi::SQLException& ex)
		{
			msg(MSG_FATAL,"IpfixDbWriterMongo: %s", ex.getMessage().c_str());	
			con->terminateStatement(stmt);
			return 1;					
		}
		msg(MSG_DEBUG,"IpfixDbWriterMongo: exporter table counter created");
		stmt->closeResultSet(rs);
		con->terminateStatement(stmt);
	}

	// create trigger
	sql.str("");	
	sql << "CREATE OR REPLACE TRIGGER trigger_for_id_exporter BEFORE INSERT ON exporter REFERENCING NEW AS NEW OLD AS OLD FOR EACH ROW Begin SELECT counter_for_exporter.NEXTVAL INTO :NEW.id FROM DUAL; End;";
	msg(MSG_DEBUG, "IpfixDbWriterMongo: SQL Query: %s", sql.str().c_str());
	try
	{
		stmt = con->createStatement(sql.str());
	}
	catch (oracle::occi::SQLException& ex)
	{
		msg(MSG_FATAL,"IpfixDbWriterMongo: %s", ex.getMessage().c_str());	
		return 1;		
	}
	if (stmt)
	{
		try 
		{
			stmt->setPrefetchRowCount(1);
			rs = stmt->executeQuery();			
		}
		catch (oracle::occi::SQLException& ex)
		{
			msg(MSG_FATAL,"IpfixDbWriterMongo: %s", ex.getMessage().c_str());	
			con->terminateStatement(stmt);
			return 1;					
		}
		msg(MSG_DEBUG,"IpfixDbWriterMongo: exporter table insert trigger created");
		stmt->closeResultSet(rs);
		con->terminateStatement(stmt);
	}	
	msg(MSG_DEBUG, "Exporter table creation done");
	return 0;
}

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
		insertStatement << "INSERT ALL INTO " << currentTable.name << " (" << tableColumnsString << ") VALUES " << rowString;
		numberOfInserts = 1;
	} else {
		// append insert statement
		insertStatement << " INTO " << currentTable.name << " (" << tableColumnsString << ") VALUES " << rowString;
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


/**
 *	loop over table columns and template to get the IPFIX values in correct order to store in database
 *	The result is written into row, the firstSwitched time is returned in flowstartsec
 */
string& IpfixDbWriterMongo::getInsertString(string& row, time_t& flowstartsec, const IpfixRecord::SourceID& sourceID,
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
	for(vector<Column>::iterator col = tableColumns.begin(); col != tableColumns.end(); col++) {
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


/*
 * Write insertStatement to database
 */
int IpfixDbWriterMongo::writeToDb()
{
	msg(MSG_DEBUG, "SQL Query: %s", insertStatement.str().c_str());
	oracle::occi::Statement *stmt = NULL;
	oracle::occi::ResultSet *rs = NULL;
	try
	{
		stmt = con->createStatement(insertStatement.str());
	}
	catch (oracle::occi::SQLException& ex)
	{
		msg(MSG_FATAL,"IpfixDbWriterMongo: %s", ex.getMessage().c_str());	
		return 1;		
	}
	if (stmt)
	{
		try 
		{
			stmt->setPrefetchRowCount(1);
			rs = stmt->executeQuery();			
		}
		catch (oracle::occi::SQLException& ex)
		{
			msg(MSG_FATAL,"IpfixDbWriterMongo: %s", ex.getMessage().c_str());	
			con->terminateStatement(stmt);
			return 1;					
		}
		stmt->closeResultSet(rs);
		con->terminateStatement(stmt);
		msg(MSG_DEBUG,"IpfixDbWriterMongo: Write to database is complete");
		return 0;
	}
	return 1;
}

/*
 * Sets the current table information and creates the table in the database if necessary
 */
int IpfixDbWriterMongo::setCurrentTable(time_t flowstartsec)
{
	// generate table name
	ostringstream tableStream;
	struct tm* flowStartTime = gmtime(&flowstartsec);

	tableStream << "H_" << (flowStartTime->tm_year+1900)
		<< setfill('0') << setw(2) << (flowStartTime->tm_mon+1)
		<< setfill('0') << setw(2) << (flowStartTime->tm_mday) << "_"
		<< setfill('0') << setw(2) << (flowStartTime->tm_hour) << "_"
		<< setw(1) << (flowStartTime->tm_min<30?0:1);

	currentTable.name = tableStream.str();

	// calculate table boundaries
	if(flowStartTime->tm_min < 30) {
		flowStartTime->tm_min = 0;
		flowStartTime->tm_sec = 0;
		currentTable.startTime = timegm(flowStartTime);
	} else {
		flowStartTime->tm_min = 30;
		flowStartTime->tm_sec = 0;
		currentTable.startTime = timegm(flowStartTime);
	}
	currentTable.endTime = currentTable.startTime + 1799;

	msg(MSG_DEBUG, "IpfixDbWriterMongo: flowstartsec: %d, table name: %s, start time: %d, end time: %d", flowstartsec, currentTable.name.c_str(), currentTable.startTime, currentTable.endTime);

	// check if table exists
	ostringstream sql;
	oracle::occi::Statement *stmt = NULL;
	oracle::occi::ResultSet *rs = NULL;
	sql << "SELECT COUNT(table_name) FROM user_tables WHERE table_name='" << currentTable.name << "'";
	msg(MSG_DEBUG, "IpfixDbWriterMongo: SQL Query: %s", sql.str().c_str());
	try
	{
		stmt = con->createStatement(sql.str());
	}
	catch (oracle::occi::SQLException& ex)
	{
		msg(MSG_FATAL,"IpfixDbWriterMongo: %s", ex.getMessage().c_str());	
		dbError = true;
		return 1;		
	}
	if (stmt)
	{
		try 
		{
			stmt->setPrefetchRowCount(1);
			rs = stmt->executeQuery();			
		}
		catch (oracle::occi::SQLException& ex)
		{
			msg(MSG_FATAL,"IpfixDbWriterMongo: %s", ex.getMessage().c_str());	
			con->terminateStatement(stmt);
			dbError = true;
			return 1;					
		}
		if (rs)
		{
			while(rs->next())
			{
				if (rs->getInt(1)!= 0) 
				{
					msg(MSG_DEBUG,"IpfixDbWriterMongo: table does exist");
					stmt->closeResultSet(rs);
					con->terminateStatement(stmt);
					return 0;	
				}
			}
			stmt->closeResultSet(rs);
		}
		con->terminateStatement(stmt);
	}

	// create table
	sql.str("");	
	sql << "CREATE TABLE " << currentTable.name << " ( " << tableColumnsCreateString << " ) TABLESPACE " << dbName;
	msg(MSG_DEBUG, "IpfixDbWriterMongo: SQL Query: %s", sql.str().c_str());
	try
	{
		stmt = con->createStatement(sql.str());
	}
	catch (oracle::occi::SQLException& ex)
	{
		msg(MSG_FATAL,"IpfixDbWriterMongo: %s", ex.getMessage().c_str());	
		dbError = true;
		return 1;		
	}
	if (stmt)
	{
		try 
		{
			stmt->setPrefetchRowCount(1);
			rs = stmt->executeQuery();			
		}
		catch (oracle::occi::SQLException& ex)
		{
			msg(MSG_FATAL,"IpfixDbWriterMongo: %s", ex.getMessage().c_str());	
			con->terminateStatement(stmt);
			dbError = true;
			return 1;					
		}
		msg(MSG_DEBUG,"IpfixDbWriterMongo: exporter table created");
		stmt->closeResultSet(rs);
		con->terminateStatement(stmt);
	}
	msg(MSG_DEBUG, "IpfixDbWriterMongo: Table %s created ", currentTable.name.c_str());
	return 0;
}


/**
 *	Returns the id of the exporter table entry or 0 in the case of an error
 */
int IpfixDbWriterMongo::getExporterID(const IpfixRecord::SourceID& sourceID)
{
	list<ExporterCacheEntry>::iterator iter;
	oracle::occi::Statement* stmt = NULL;
	oracle::occi::ResultSet* rs = NULL;
	int id = -1;
	uint32_t expIp = 0;
	ostringstream sql;

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

	// search exporter table
	sql << "SELECT id FROM exporter WHERE sourceID=" << sourceID.observationDomainId << " AND srcIp=" << expIp;
	msg(MSG_DEBUG, "IpfixDbWriterMongo: SQL Query: %s", sql.str().c_str());
	try 
	{
		stmt = con->createStatement(sql.str());
	}
	catch (oracle::occi::SQLException &ex)
	{
		msg(MSG_ERROR,"IpfixDbWriterMongo: Select on exporter table failed. Error: %s", ex.getMessage().c_str());	
		return 0;// If a failure occurs, return 0
	}
	if(stmt)
	{
		try 
		{
			stmt->setPrefetchRowCount(1);
			rs = stmt->executeQuery();
			if (rs)
			{
				while(rs->next())
				{
					id = rs->getInt(1);
					msg(MSG_DEBUG, "IpfixDbWriterMongo: ExporterID %d is in exporter table", id);					
				}
				stmt->closeResultSet(rs);
			}
			con->terminateStatement(stmt);
		}
		catch (oracle::occi::SQLException &ex)
		{
			msg(MSG_ERROR,"IpfixDbWriterMongo: Select on exporter table failed. Error: %s", ex.getMessage().c_str());	
			con->terminateStatement(stmt);
			return 0;// If a failure occurs, return 0			
		}
	}
	// insert new entry in exporter table since it is not found
	if(id == -1)
	{
		sql.str("");
		sql << "INSERT INTO exporter (ID,sourceID,srcIP) VALUES ( 0 ,'" << sourceID.observationDomainId << "','" << expIp << "')";
		msg(MSG_DEBUG, "IpfixDbWriterMongo: SQL Query: %s", sql.str().c_str());
		try
		{
			stmt = con->createStatement(sql.str());
		}
		catch (oracle::occi::SQLException& ex)
		{
			msg(MSG_ERROR,"IpfixDbWriterMongo: Insert in exporter table failed. Error: %s", ex.getMessage().c_str());	
			return 0;		
		}
		if (stmt)
		{
			try 
			{
				stmt->setPrefetchRowCount(1);
				rs = stmt->executeQuery();			
			}
			catch (oracle::occi::SQLException& ex)
			{
				msg(MSG_FATAL,"IpfixDbWriterMongo: Insert in exporter table failed. Error: %s", ex.getMessage().c_str());	
				con->terminateStatement(stmt);
				return 0;					
			}
			stmt->closeResultSet(rs);
			con->terminateStatement(stmt);
		}

		sql.str("");
		sql << "SELECT counter_for_exporter.CURRVAL FROM DUAL";
		msg(MSG_DEBUG, "IpfixDbWriterMongo: SQL Query: %s", sql.str().c_str());
		try 
		{
			stmt = con->createStatement(sql.str());
		}
		catch (oracle::occi::SQLException &ex)
		{
			msg(MSG_ERROR,"IpfixDbWriterMongo: Select on counter_for_exporter sequence failed. Error: %s", ex.getMessage().c_str());	
			return 0;// If a failure occurs, return 0
		}
		if(stmt)
		{
			try 
			{
				stmt->setPrefetchRowCount(1);
				rs = stmt->executeQuery();
				if (rs)
				{
					while(rs->next())
					{
						id = rs->getInt(1)
						DPRINTF("ExporterID %d is in exporter table", id);
					}
					stmt->closeResultSet(rs);
				}
				con->terminateStatement(stmt);
			}
			catch (oracle::occi::SQLException &ex)
			{
				msg(MSG_ERROR,"IpfixDbWriterMongo: Select on counter_for_exporter sequence failed. Error: %s", ex.getMessage().c_str());	
				con->terminateStatement(stmt);
				return 0;// If a failure occurs, return 0			
			}
			msg(MSG_INFO,"IpfixDbWriter: new exporter (ODID=%d, id=%d) inserted in exporter table", sourceID.observationDomainId, id);		
		}
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
IpfixDbWriterMongo::IpfixDbWriterMongo(const string& hostname, const string& dbname,
		const string& username, const string& password,
		unsigned port, uint32_t observationDomainId, unsigned maxStatements,
		const vector<string>& columns)
	: currentExporter(NULL), numberOfInserts(0), maxInserts(maxStatements),
	dbHost(hostname), dbName(dbname), dbUser(username), dbPassword(password), dbPort(port), con(0)
{
	int i;

	// set default source id
	srcId.exporterAddress.len = 0;
	srcId.observationDomainId = observationDomainId;
	srcId.exporterPort = 0;
	srcId.receiverPort = 0;
	srcId.protocol = 0;
	srcId.fileDescriptor = 0;

	// invalide start settings for current table (to enforce table create)
	currentTable.startTime = 1;
	currentTable.endTime = 0;

	if(columns.empty())
		THROWEXCEPTION("IpfixDbWriter: cannot initiate with no columns");

	/* get columns */
	bool first = true;
	for(vector<string>::const_iterator col = columns.begin(); col != columns.end(); col++) {
		i = 0;
		while(identify[i].columnName != 0) {
			if(col->compare(identify[i].columnName) == 0) {
				Column c = identify[i];
				tableColumns.push_back(c);
				// update tableColumnsString
				if(!first)
					tableColumnsString.append(",");
				tableColumnsString.append(identify[i].columnName);
				// update tableColumnsCreateString
				if(!first)
					tableColumnsCreateString.append(", ");
				tableColumnsCreateString.append(identify[i].columnName);
				tableColumnsCreateString.append(" ");
				tableColumnsCreateString.append(identify[i].columnType);
				first = false;
				break;
			}
			i++;
		}
	}
	msg(MSG_INFO, "IpfixDbWriter: columns are %s", tableColumnsString.c_str());

	if(connectToDB() != 0)
		THROWEXCEPTION("IpfixDbWriter creation failed");
}


/**
 * Destructor
 */
IpfixDbWriterMongo::~IpfixDbWriterMongo()
{
	writeToDb();
	env->terminateConnection(con);
	oracle::occi::Environment::terminateEnvironment(env);
}



#endif /* MONGO_SUPPORT_ENABLED */
