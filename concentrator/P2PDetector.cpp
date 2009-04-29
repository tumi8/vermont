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

#include <math.h>
#include <iostream>


InstanceManager<IDMEFMessage> P2PDetector::idmefManager("IDMEFMessage");

//Threasholds for the criteria
const double udpRateThreshold = 0.1;  // uni-dumps: 4
const double udpHostRateThreshold = 0.3;
const double tcpRateThreshold = 0.25;
const double coexistentTCPConsThreshold = 14; // uni-dumps: 22
const double rateLongTCPConsThreshold = 0.1; //home: 0.05; uni2: 0.2
const double tcpVarianceThreshold = 1.3; //home: 1,67; uni2: 1.3
const double failedConsPercentThreshold = 5; //home: 3
const double tcpFailedRateThreshold = 0.07; // home: 0.014 uni1: 0.12
const double tcpFailedVarianceThreshold = 1.22;

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
 */
void P2PDetector::onDataDataRecord(IpfixDataDataRecord* record)
{
	// convert ipfixrecord to connection struct
	Connection conn(record);
	conn.swapIfNeeded();
	
	if(conn.srcTimeStart == 0)
		cout << "null" << endl;
	//cout << IPToString(conn.srcIP) << "->" << IPToString(conn.dstIP) << ": " << (int)conn.protocol << endl;
	
	if((conn.srcIP & subnetmask) == (subnet & subnetmask)){
		P2PEntry& entry = hostList[conn.srcIP];
		//UDP biflows
		if(conn.protocol == 17){
			//number of UDP biflows
			entry.numUDPBiFlows++;
			//contacted UDP hosts
			entry.contactedUDPHosts[conn.dstIP] = true;
		}
		//TCP biflows
		if(conn.protocol == 6){
			//number of all TCP biflows
			entry.numTCPBiFlows++;
			if(succConn(conn)){
				uint64_t flowLength = (conn.srcTimeEnd < conn.dstTimeEnd ? conn.dstTimeEnd : conn.srcTimeEnd) - conn.srcTimeStart;
				//sum of all biflow length 
				entry.sumTCPLength += flowLength;
				//number of long TCP connections
				if(flowLength >= 60000)
					entry.numLongTCPCons++;
				//list of all starting points of the biflows to calculate the variance of starting points
				entry.succBiFlowStarts.push_back(conn.srcTimeStart);
			}else{
				//number of failed TCP connections
				entry.numFailedTCPCons++;
				//list of all starting points of the failed connections to calculate the variance of failed connection attempts
				entry.failedBiFlowStarts.push_back(conn.srcTimeStart);
			}
		}
		
	}
	
	if((conn.dstIP & subnetmask) == (subnet & subnetmask)){
		P2PEntry& entry = hostList[conn.dstIP];
		//UDP biflows
		if(conn.protocol == 17){
			//number of UDP biflows
			entry.numUDPBiFlows++;
		}
		//TCP biflows
		if(conn.protocol == 6){
			//number of all TCP biflows
			entry.numTCPBiFlows++;
			if(succConn(conn)){
				uint64_t flowLength = (conn.srcTimeEnd < conn.dstTimeEnd ? conn.dstTimeEnd : conn.srcTimeEnd) - conn.srcTimeStart;
				//sum of all biflow length 
				entry.sumTCPLength += flowLength;
				//number of long TCP connections
				if(flowLength >= 60000)
					entry.numLongTCPCons++;
				//list of all starting points of the biflows to calculate the variance of starting points
				entry.succBiFlowStarts.push_back(conn.srcTimeStart);
			}else{
				//number of failed TCP connections
				entry.numFailedTCPCons++;
				//list of all starting points of the failed connections to calculate the variance of failed connection attempts
				entry.failedBiFlowStarts.push_back(conn.srcTimeStart);
			}
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

	addToCurTime(&nextTimeout, intLength*1000);
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
	//criterias
	double udpRate;
	double udpHostRate;
	double tcpRate;
	double coexistentTCPCons;
	double rateLongTCPCons;
	double tcpVariance;
	double failedConsPercent;
	double tcpFailedRate;
	double tcpFailedVariance;
	
	//loop through all entries
	for(map<uint32_t, P2PEntry>::iterator iter = hostList.begin(); iter != hostList.end(); iter++){
		udpRate = ((double)(iter->second.numUDPBiFlows)) / intLength;
		udpHostRate = ((double)(iter->second.contactedUDPHosts.size())) / intLength;
		tcpRate = ((double)(iter->second.numTCPBiFlows)) / intLength;
		coexistentTCPCons = ((double)(iter->second.sumTCPLength)) / intLength;
		rateLongTCPCons = ((double)(iter->second.numLongTCPCons)) / intLength;
		//tcpVariance
		if(iter->second.succBiFlowStarts.size() < 3)
			tcpVariance = -1;
		else{
			uint64_t sum = 0;
			uint64_t qsum = 0;
			double variance;
			list<uint64_t>::iterator ptr1 = iter->second.succBiFlowStarts.begin();
			list<uint64_t>::iterator ptr2 = ptr1++;
			
			iter->second.succBiFlowStarts.sort();
			for(; ptr1 != iter->second.succBiFlowStarts.end(); ptr1++, ptr2++){
				sum += *ptr1 - *ptr2;
				qsum += (*ptr1 - *ptr2) * (*ptr1 - *ptr2);
			}
			//sample variance (stichprobenvarianz) /  n = succBiFlowStarts.size()-1: the differences not the starting points itself 
			variance = (1.0/(iter->second.succBiFlowStarts.size()-2))*(qsum - ((1.0/(iter->second.succBiFlowStarts.size()-1))*(sum*sum)));
			tcpVariance = sqrt(variance)/(iter->second.succBiFlowStarts.size()-1);	
		}
		failedConsPercent = (((double)(iter->second.numFailedTCPCons)) * 100) / iter->second.numTCPBiFlows;
		tcpFailedRate = ((double)(iter->second.numFailedTCPCons)) / intLength;
		//variance of failed connections
		if(iter->second.failedBiFlowStarts.size() < 3)
			tcpFailedVariance = -1;
		else{
			uint64_t sum = 0;
			uint64_t qsum = 0;
			double variance;
			list<uint64_t>::iterator ptr1 = iter->second.failedBiFlowStarts.begin();
			list<uint64_t>::iterator ptr2 = ptr1++;
			
			iter->second.failedBiFlowStarts.sort();
			for(; ptr1 != iter->second.failedBiFlowStarts.end(); ptr1++, ptr2++){
				sum += *ptr1 - *ptr2;
				qsum += (*ptr1 - *ptr2) * (*ptr1 - *ptr2);
			}
			//sample variance (stichprobenvarianz) /  n = failedBiFlowStarts.size()-1: the differences not the starting points itself 
			variance = (1.0/(iter->second.failedBiFlowStarts.size()-2))*(qsum - ((1.0/(iter->second.failedBiFlowStarts.size()-1))*(sum*sum)));
			tcpFailedVariance = sqrt(variance)/(iter->second.failedBiFlowStarts.size()-1);	
		}
		
		//decide wether researched host is peer-to-peer or not
		int points = 0;
		if(udpRate > udpRateThreshold)
			points++;
		if(udpHostRate > udpHostRateThreshold)
			points++;
		if(tcpRate > tcpRateThreshold)
			points++;
		if(coexistentTCPCons > coexistentTCPConsThreshold)
			points++;
		if(rateLongTCPCons > rateLongTCPConsThreshold)
			points++;
		if(tcpVariance > tcpVarianceThreshold)
			points++;
		if(failedConsPercent > failedConsPercentThreshold)
			points++;
		if(tcpFailedRate > tcpFailedRateThreshold)
			points++;
		if(tcpFailedVariance > tcpFailedVariance)
			points++;
		
		//host is a p2p client	
		if(points > 6){
			//send Messag
			msg(MSG_DEBUG, "P2P client detected:");
			msg(MSG_DEBUG, "IP: %s, dstSubnet: %s, dstSubMask: %s", IPToString(iter->first).c_str(), 
				IPToString(subnet).c_str(), IPToString(subnetmask).c_str());
			//msg(MSG_DEBUG, "numFailedConns: %d, numSuccConns: %d", te->numFailedConns, te->numSuccConns);
			
			IDMEFMessage* msg = idmefManager.getNewInstance();
			msg->init(idmefTemplate, analyzerId);
			msg->setVariable("SUCC_CONNS", "42");
			msg->setVariable("FAILED_CONNS", "42");
			msg->setVariable(IDMEFMessage::PAR_SOURCE_ADDRESS, "192.168.1.1");
			msg->setVariable(IDMEFMessage::PAR_TARGET_ADDRESS, IPToString(subnet)+"/"+IPToString(subnetmask));
			msg->applyVariables();
			send(msg);
		}
		
	}
	

	
}

/**
 * things to be done when the module is shutdown
 */
void P2PDetector::performShutdown()
{
	onTimeout(NULL);
	hostList.clear();
}

/**
 * decides wether a connection was successfull or only a connection attempt
 */
bool P2PDetector::succConn(Connection& conn){
	
//	cout << "***Connection:\n";
//	cout << IPToString(conn.srcIP) << "->" << IPToString(conn.dstIP) << "\n";
//	cout << "start: " << conn.srcTimeStart << " end: " << conn.srcTimeEnd << " | " << conn.dstTimeEnd << "\n";
//	cout << "srcOctets: " << (long long)ntohll(conn.srcOctets) << " dstOctets: " << (long long)ntohll(conn.dstOctets) << "\n";
//	cout << "srcPackets: " << (long long)ntohll(conn.srcPackets) << " dstPackets: " << (long long)ntohll(conn.dstPackets) << "\n";
//	cout << "srcTCP: " << hex << (int)conn.srcTcpControlBits << " dstTCP: " << (int)conn.dstTcpControlBits << dec << endl;
//	
	
	//packets must be send in both directions
	if((conn.srcPackets != 0) && (conn.dstPackets != 0)){
		//more data were transmitted then just header informations
		if(((ntohll(conn.srcOctets) / ntohll(conn.srcPackets)) > 40) || ((ntohll(conn.dstOctets) / ntohll(conn.dstPackets)) > 40)){
			//full 3-way-handshake exists and src aborted the connection abrupt
			if((ntohll(conn.dstPackets) > 1) && (ntohll(conn.srcPackets) > 2) && 
				((conn.dstTcpControlBits & (Connection::SYN | Connection::ACK)) == (Connection::SYN | Connection::ACK)) && 
				((conn.srcTcpControlBits & (Connection::SYN | Connection::ACK | Connection::RST)) == (Connection::SYN | Connection::ACK | Connection::RST)))
					//{cout << "conntype-a" << endl; return true; }
					return true;
					
			//full 3-way-handshake eixts and dst aborted the connection abrupt
			if((ntohll(conn.dstPackets) > 2) && (ntohll(conn.srcPackets) > 1) && 
				((conn.dstTcpControlBits & (Connection::SYN | Connection::ACK | Connection::RST)) == (Connection::SYN | Connection::ACK | Connection::RST)) && 
				((conn.srcTcpControlBits & (Connection::SYN | Connection::ACK)) == (Connection::SYN | Connection::ACK)))
					//{ cout << "conntype-b" << endl; return true;}
					return true;
					
			//complete established and regularly closed connection from both sides
			if(((conn.dstTcpControlBits & (Connection::SYN | Connection::ACK | Connection::FIN)) == (Connection::SYN | Connection::ACK | Connection::FIN)) && 
				((conn.srcTcpControlBits & (Connection::SYN | Connection::ACK | Connection::FIN)) == (Connection::SYN | Connection::ACK | Connection::FIN)) && 
				(((ntohll(conn.dstPackets) > 2) && (ntohll(conn.srcPackets) > 3)) || ((ntohll(conn.dstPackets) > 3) && (ntohll(conn.srcPackets) > 2))))
					//{ cout << "conntype-c" << endl; return true;}
					return true;
			
			//if creation and clearing of connection is not in the researched interval
			if(
				(
					((conn.srcTcpControlBits & Connection::ACK) == Connection::ACK) ||
					((conn.srcTcpControlBits & Connection::ACK) == Connection::ACK)
				) &&
				(conn.dstTimeEnd != 0) &&
				(
					//connection lasts at least 60 seconds 
					(conn.srcTimeEnd - conn.srcTimeStart >= 60000) ||
					(conn.dstTimeEnd - conn.srcTimeStart >= 60000) 				
				)
			)
					//{ cout << "conntype-d" << endl; return true;}
					return true;
			
		}
	}
	//cout << "connection attempt" << endl;
	return false;
}