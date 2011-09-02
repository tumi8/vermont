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
	env = Environment::createEnvironment(Environment::DEFAULT);
	try {
		char dbLogon[128];
		sprintf(server, "%s:%u/", dbHost, dbPort);
		con = env->createConnection(dbUser, dbPassword, dbLogon)
	} catch (SQLException& ex)
		msg(MSG_FATAL,"IpfixDbWriterOracle: Oracle connect failed. Error: %s", ex.getMessage());
		return 1;
	}
	msg(MSG_DEBUG,"IpfixDbWriterOracle: Oracle connection successful");

	return 0;
}

/**
 * Destructor
 */
IpfixDbWriterOracle::~IpfixDbWriter()
{
	writeToDb();
	env->terminateConnection(con);
	Environment::terminateEnvironment(env);
}



#endif /* ORACLE_SUPPORT_ENABLED */