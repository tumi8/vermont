/*
 * IPFIX Database Reader/Writer Oracle Connector
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

#ifdef ORACLE_SUPPORT_ENABLED

#include <stdexcept>
#include <string.h>
#include <iomanip>
#include <stdlib.h>
#include "IpfixDbWriterOracle.hpp"
#include "common/msg.h"

IpfixDbWriterOracle::Column identify [] = {
	{CN_dstIP, 		"INTEGER(10) UNSIGNED", 	0, IPFIX_TYPEID_destinationIPv4Address, 0},
	{CN_srcIP, 		"INTEGER(10) UNSIGNED", 	0, IPFIX_TYPEID_sourceIPv4Address, 0},
	{CN_srcPort, 		"SMALLINT(5) UNSIGNED", 	0, IPFIX_TYPEID_sourceTransportPort, 0},
	{CN_dstPort, 		"SMALLINT(5) UNSIGNED", 	0, IPFIX_TYPEID_destinationTransportPort, 0},
	{CN_proto, 		"TINYINT(3) UNSIGNED", 		0, IPFIX_TYPEID_protocolIdentifier, 0 },
	{CN_dstTos, 		"TINYINT(3) UNSIGNED", 		0, IPFIX_TYPEID_classOfServiceIPv4, 0},
	{CN_bytes, 		"BIGINT(20) UNSIGNED", 		0, IPFIX_TYPEID_octetDeltaCount, 0},
	{CN_pkts, 		"BIGINT(20) UNSIGNED", 		0, IPFIX_TYPEID_packetDeltaCount, 0},
	{CN_firstSwitched, 	"INTEGER(10) UNSIGNED", 	0, IPFIX_TYPEID_flowStartSeconds, 0}, // default value is invalid/not used for this ent
	{CN_lastSwitched, 	"INTEGER(10) UNSIGNED", 	0, IPFIX_TYPEID_flowEndSeconds, 0}, // default value is invalid/not used for this entry
	{CN_firstSwitchedMillis, "SMALLINT(5) UNSIGNED", 	0, IPFIX_TYPEID_flowStartMilliSeconds, 0},
	{CN_lastSwitchedMillis, "SMALLINT(5) UNSIGNED", 	0, IPFIX_TYPEID_flowEndMilliSeconds, 0},
	{CN_tcpControlBits,  	"SMALLINT(5) UNSIGNED", 	0, IPFIX_TYPEID_tcpControlBits, 0},
	//TODO: use enterprise number for the following extended types (Gerhard, 12/2009)
	{CN_revbytes, 		"BIGINT(20) UNSIGNED", 		0, IPFIX_TYPEID_octetDeltaCount, IPFIX_PEN_reverse},
	{CN_revpkts, 		"BIGINT(20) UNSIGNED", 		0, IPFIX_TYPEID_packetDeltaCount, IPFIX_PEN_reverse},
	{CN_revFirstSwitched, 	"INTEGER(10) UNSIGNED", 	0, IPFIX_TYPEID_flowStartSeconds, IPFIX_PEN_reverse}, // default value is invalid/not used for this entry
	{CN_revLastSwitched, 	"INTEGER(10) UNSIGNED", 	0, IPFIX_TYPEID_flowEndSeconds, IPFIX_PEN_reverse}, // default value is invalid/not used for this entry
	{CN_revFirstSwitchedMillis, "SMALLINT(5) UNSIGNED", 	0, IPFIX_TYPEID_flowStartMilliSeconds, IPFIX_PEN_reverse},
	{CN_revLastSwitchedMillis, "SMALLINT(5) UNSIGNED", 	0, IPFIX_TYPEID_flowEndMilliSeconds, IPFIX_PEN_reverse},
	{CN_revTcpControlBits,  "SMALLINT(5) UNSIGNED", 	0, IPFIX_TYPEID_tcpControlBits, IPFIX_PEN_reverse},
	{CN_maxPacketGap,  	"BIGINT(20) UNSIGNED", 		0, IPFIX_ETYPEID_maxPacketGap, IPFIX_PEN_vermont|IPFIX_PEN_reverse},
	{CN_exporterID, 	"SMALLINT(5) UNSIGNED", 	0, EXPORTERID, 0},
	{0} // last entry must be 0
};

/**
 * Compare two source IDs and check if exporter is the same (i.e., same IP address and observationDomainId
 */
bool IpfixDbWriterOracle::equalExporter(const IpfixRecord::SourceID& a, const IpfixRecord::SourceID& b) {
	return (a.observationDomainId == b.observationDomainId) &&
		(a.exporterAddress.len == b.exporterAddress.len) &&
		(memcmp(a.exporterAddress.ip, b.exporterAddress.ip, a.exporterAddress.len) == 0 );
}

/**
 * (re)connect to database
 */
int IpfixDbWriterOracle::connectToDB()
{
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
		msg(MSG_FATAL,"IpfixDbWriterOracle: Oracle connect failed. Error: %s", ex.getMessage().c_str());
		return 1;
	}
	msg(MSG_DEBUG,"IpfixDbWriterOracle: Oracle connection successful");
	
	if (createExporterTable()!=0) return 1;
	
	return 0;
}

int IpfixDbWriterOracle::createExporterTable()
{
	/** create table exporter*/
	ostringstream sql, sql2;
	oracle::occi::Statement *stmt = NULL;
	oracle::occi::Statement *stmt2 = NULL;
	oracle::occi::ResultSet *rs = NULL;
	oracle::occi::ResultSet *rs2 = NULL;
	sql << "SELECT COUNT(table_name) FROM user_tables WHERE table_name='EXPORTER'";
	try 
	{
		stmt = con->createStatement(sql.str());
	}
	catch (oracle::occi::SQLException& ex)
	{
		msg(MSG_FATAL,"IpfixDbWriterOracle: %s", ex.getMessage().c_str());	
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
			stmt->closeResultSet(rs);
			con->terminateStatement(stmt);
			msg(MSG_FATAL,"IpfixDbWriterOracle: %s", ex.getMessage().c_str());	
			return 1;
		}
		if (rs)
		{
			while(rs->next())
			{
				if (rs->getInt(1) == 0)
				{
					sql2 << "CREATE TABLE exporter ( id NUMERIC(10) NOT NULL, sourceID NUMERIC(10), srcIP NUMERIC(10), CONSTRAINT exporter_pk PRIMARY KEY (id) ) TABLESPACE " << dbName;
					try 
					{
						stmt2 = con->createStatement(sql2.str());
					}
					catch (oracle::occi::SQLException& ex)
					{
						stmt->closeResultSet(rs);
						con->terminateStatement(stmt);
						msg(MSG_FATAL,"IpfixDbWriterOracle: %s", ex.getMessage().c_str());	
						return 1;
					}
					if (stmt2)
					{						
						try 
						{
							stmt2->setPrefetchRowCount(1);
							rs2 = stmt2->executeQuery();
						}
						catch (oracle::occi::SQLException& ex)
						{
							msg(MSG_FATAL,"IpfixDbWriterOracle: %s", ex.getMessage().c_str());	
							stmt->closeResultSet(rs);
							con->terminateStatement(stmt);
							stmt2->closeResultSet(rs2);
							con->terminateStatement(stmt2);
							return 1;
						}
						msg(MSG_DEBUG, "Exporter table created");
						stmt2->closeResultSet(rs2);
						con->terminateStatement(stmt2);												
					}
				}
				else 
				{
					msg(MSG_DEBUG,"IpfixDbWriterOracle: exporter table does exist");				
				}			
			}
			stmt->closeResultSet(rs);
		}
		con->terminateStatement(stmt);
	}
	return 0;
}




/**
 * save record to database
 */
void IpfixDbWriterOracle::processDataDataRecord(const IpfixRecord::SourceID& sourceID,
		TemplateInfo& dataTemplateInfo, uint16_t length,
		IpfixRecord::Data* data)
{
	
	// FIXME Missing
	
}


/**
 *	loop over table columns and template to get the IPFIX values in correct order to store in database
 *	The result is written into row, the firstSwitched time is returned in flowstartsec
 */
string& IpfixDbWriterOracle::getInsertString(string& row, time_t& flowstartsec, const IpfixRecord::SourceID& sourceID,
		TemplateInfo& dataTemplateInfo,uint16_t length, IpfixRecord::Data* data)
{
	// FIXME Missing
	return row;
}


/*
 * Write insertStatement to database
 */
int IpfixDbWriterOracle::writeToDb()
{
	// FIXME Missing
	return 0; // error
}


/**
 *	Returns the id of the exporter table entry or 0 in the case of an error
 */
int IpfixDbWriterOracle::getExporterID(const IpfixRecord::SourceID& sourceID)
{
	// FIXME Missing
	return 0; // error
}

/**
 *	Get data of the record is given by the IPFIX_TYPEID
 */
uint64_t IpfixDbWriterOracle::getData(InformationElement::IeInfo type, IpfixRecord::Data* data)
{
	return 0; // error
}

/***** Public Methods ****************************************************/

/**
 * called on Data Record arrival
 */
void IpfixDbWriterOracle::onDataRecord(IpfixDataRecord* record)
{
	
}

/**
 * Constructor
 */
IpfixDbWriterOracle::IpfixDbWriterOracle(const string& hostname, const string& dbname,
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
IpfixDbWriterOracle::~IpfixDbWriterOracle()
{
	writeToDb();
	env->terminateConnection(con);
	oracle::occi::Environment::terminateEnvironment(env);
}



#endif /* ORACLE_SUPPORT_ENABLED */