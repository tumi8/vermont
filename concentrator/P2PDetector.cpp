/*
 * VERMONT 
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
 */

#include "P2PDetector.h"
#include "common/Misc.h"
#include "common/Time.h"
#include "reconf/Timer.h"

#include <iostream>


InstanceManager<IDMEFMessage> P2PDetector::idmefManager("IDMEFMessage");

/**
 * P2PDetector
 */
P2PDetector::P2PDetector(uint32_t intLength, uint32_t subnet, uint32_t subnetmask, string analyzerid, string tpl):
	intLength(intLength),
	subnet(subnet),
	subnetmask(subnetmask),
	analyzerId(analyzerid),
	idmefTemplate(tpl),
	timeoutRegistered(false)
{
}

P2PDetector::~P2PDetector()
{
	//cout << "intLength: " << intLength << " subnet: " << IPToString(subnet) << " subnetmask: " << IPToString(subnetmask) << endl;
	//for(map<uint32_t, P2PEntry>::iterator iter = hostList.begin(); iter != hostList.end(); iter++){
	//	cout << IPToString(iter->first) << ": " << iter->second.numPkt << endl;
	//} 
}

/**
 * Get new Biflows from the aggregator 
 * 
 */
void P2PDetector::onDataDataRecord(IpfixDataDataRecord* record)
{
	// convert ipfixrecord to connection struct
	Connection conn(record);
	conn.swapIfNeeded();
	
	cout << IPToString(conn.srcIP) << "->" << IPToString(conn.dstIP) << ": " << (int)conn.protocol << endl;
	
	if((conn.srcIP & subnetmask) == (subnet & subnetmask)){
		//UDP biflows
		if(conn.protocol == 17){
			//number of udp biflows
			hostList[conn.srcIP].numUDPBiFlows++;
			//contacted udp hosts
			hostList[conn.srcIP].contactedUDPHosts[conn.dstIP] = true;
		}
		//TCP biflows
		if(conn.protocol == 6){
			hostList[conn.srcIP].numTCPBiFlows++;
		}
		
	}
	
	if((conn.dstIP & subnetmask) == (subnet & subnetmask)){
		//UDP biflows
		if(conn.protocol == 17){
			//number of udp biflows
			hostList[conn.dstIP].numUDPBiFlows++;
		}
		//TCP biflows
		if(conn.protocol == 6){
			hostList[conn.dstIP].numTCPBiFlows++;
		}

	}
	
	
	
	record->removeReference();
	
	registerTimeout();
}

/**
 * registers timeout for function onTimeout in Timer
 * to compute the criterias for every interval
 */
void P2PDetector::registerTimeout()
{
	if (timeoutRegistered) return;

	addToCurTime(&nextTimeout, intLength);
	timer->addTimeout(this, nextTimeout, NULL);
	timeoutRegistered = true;
}

/**
 * 
 * 
 */
void P2PDetector::onTimeout(void* dataPtr)
{
	timeoutRegistered = false;
	cout << "bla" << endl;
	msg(MSG_DEBUG, "P2P client detected:");
	//msg(MSG_DEBUG, "srcIP: %s, dstSubnet: %s, dstSubMask: %s", IPToString(te->srcIP).c_str(), 
	//	IPToString(te->dstSubnet).c_str(), IPToString(te->dstSubnetMask).c_str());
	//msg(MSG_DEBUG, "numFailedConns: %d, numSuccConns: %d", te->numFailedConns, te->numSuccConns);

//	IDMEFMessage* msg = idmefManager.getNewInstance();
//	msg->init(idmefTemplate, analyzerId);
//	msg->setVariable("SUCC_CONNS", "42");
//	msg->setVariable("FAILED_CONNS", "42");
//	msg->setVariable(IDMEFMessage::PAR_SOURCE_ADDRESS, "192.168.1.1");
//	msg->setVariable(IDMEFMessage::PAR_TARGET_ADDRESS, IPToString(subnet)+"/"+IPToString(subnetmask));
//	msg->applyVariables();
//	send(msg);
	
}

void P2PDetector::performShutdown()
{
	onTimeout(NULL);
	hostList.clear();
}

bool P2PDetector::succConn(Connection& conn){
	
	cout << "***Connection:\n";
	cout << IPToString(conn.srcIP) << "->" << IPToString(conn.dstIP) << "\n";
	cout << "start: " << (long long)ntohll(conn.srcTimeStart) << " end: " << (long long)ntohll(conn.srcTimeEnd) << " | " << (long long)ntohll(conn.dstTimeEnd) << "\n";
	cout << "srcOctets: " << (long long)ntohll(conn.srcOctets) << " dstOctets: " << (long long)ntohll(conn.dstOctets) << "\n";
	cout << "srcPackets: " << (long long)ntohll(conn.srcPackets) << " dstPackets: " << (long long)ntohll(conn.dstPackets) << "\n";
	cout << "srcTCP: " << hex << conn.srcTcpControlBits << " dstTCP: " << conn.dstTcpControlBits << dec << endl;
	
	
	//packets must be send in both directions
	if((conn.srcPackets != 0) && (conn.dstPackets != 0)){
		//more data were transmitted then just header informations
		if(((ntohll(conn.srcOctets) / ntohll(conn.srcPackets)) > 40) || ((ntohll(conn.dstOctets) / ntohll(conn.dstPackets)) > 40)){
			//full 3-way-handshake exists and src aborted the connection abrupt
			if((ntohll(conn.dstPackets) > 1) && (ntohll(conn.srcPackets) > 2) && 
				((conn.dstTcpControlBits & (Connection::SYN | Connection::ACK)) == (Connection::SYN | Connection::ACK)) && 
				((conn.srcTcpControlBits & (Connection::SYN | Connection::ACK | Connection::RST)) == (Connection::SYN | Connection::ACK | Connection::RST)))
					{return true; cout << "a" << endl;}
					
			//full 3-way-handshake eixts and dst aborted the connection abrupt
			if((ntohll(conn.dstPackets) > 2) && (ntohll(conn.srcPackets) > 1) && 
				((conn.dstTcpControlBits & (Connection::SYN | Connection::ACK | Connection::RST)) == (Connection::SYN | Connection::ACK | Connection::RST)) && 
				((conn.srcTcpControlBits & (Connection::SYN | Connection::ACK)) == (Connection::SYN | Connection::ACK)))
					{return true; cout << "b" << endl;}
					
			//complete established and regularly closed connection from both sides
			if(((conn.dstTcpControlBits & (Connection::SYN | Connection::ACK | Connection::FIN)) == (Connection::SYN | Connection::ACK | Connection::FIN)) && 
				((conn.srcTcpControlBits & (Connection::SYN | Connection::ACK | Connection::FIN)) == (Connection::SYN | Connection::ACK | Connection::FIN)) && 
				(((ntohll(conn.dstPackets) > 2) && (ntohll(conn.srcPackets) > 3)) || ((ntohll(conn.dstPackets) > 3) && (ntohll(conn.srcPackets) > 2))))
					{return true; cout << "c" << endl;}
					
			//complete established and regularly closed connection from both sides
			if(((conn.dstTcpControlBits & (Connection::SYN | Connection::ACK | Connection::FIN)) == (Connection::SYN | Connection::ACK | Connection::FIN)) && 
				((conn.srcTcpControlBits & (Connection::SYN | Connection::ACK | Connection::FIN)) == (Connection::SYN | Connection::ACK | Connection::FIN)) && 
				(((ntohll(conn.dstPackets) > 2) && (ntohll(conn.srcPackets) > 3)) || ((ntohll(conn.dstPackets) > 3) && (ntohll(conn.srcPackets) > 2))))
					{return true; cout << "d" << endl;}
			
			//if creation and clearing of connection is not in the researched interval
			if(
				(
					((conn.srcTcpControlBits & Connection::ACK) == Connection::ACK) ||
					((conn.srcTcpControlBits & Connection::ACK) == Connection::ACK)
				) &&
				(
					//connection lasts at least 60 seconds 
					(conn.srcTimeEnd - conn.srcTimeStart >= 60000) ||
					(conn.dstTimeEnd - conn.srcTimeStart >= 60000) 				
				)
			)
					{return true; cout << "e" << endl;}
			
		}
	}
	cout << "connection attempt" << endl;
	return false;
}