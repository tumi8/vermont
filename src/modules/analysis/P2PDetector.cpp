/*
 * VERMONT
 * Copyright (C) 2009 Simon Regnet <s-regnet@user.berlios.de>
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
#include "core/Timer.h"

#include <math.h>
#include <iostream>


InstanceManager<IDMEFMessage> P2PDetector::idmefManager("P2PIDMEFMessage", 0);


/**
 * P2PDetector stes the values given by the corresponding configuration class
 */
P2PDetector::P2PDetector(uint32_t intLength, uint32_t subnet, uint32_t subnetmask, string analyzerid, string tpl,
				double udpRateThreshold, double udpHostRateThreshold, double tcpRateThreshold, double coexistentTCPConsThreshold,
				double rateLongTCPConsThreshold, double tcpVarianceThreshold, double failedConsPercentThreshold,
				double tcpFailedRateThreshold, double tcpFailedVarianceThreshold):
	timeoutRegistered(false),
	intLength(intLength),
	subnet(subnet),
	subnetmask(subnetmask),
	udpRateThreshold(udpRateThreshold),
	udpHostRateThreshold(udpHostRateThreshold),
	tcpRateThreshold(tcpRateThreshold),
	coexistentTCPConsThreshold(coexistentTCPConsThreshold),
	rateLongTCPConsThreshold(rateLongTCPConsThreshold),
	tcpVarianceThreshold(tcpVarianceThreshold),
	failedConsPercentThreshold(failedConsPercentThreshold),
	tcpFailedRateThreshold(tcpFailedRateThreshold),
	tcpFailedVarianceThreshold(tcpFailedVarianceThreshold),
	analyzerId(analyzerid),
	idmefTemplate(tpl)
{
}

P2PDetector::~P2PDetector()
{
}

/**
 * Gets new Biflows from the aggregator and collects these data
 */
void P2PDetector::onDataRecord(IpfixDataRecord* record)
{
	// only treat non-Options Data Records (although we cannot be sure that there is a Flow inside)
	if((record->templateInfo->setId != TemplateInfo::NetflowTemplate) 
		&& (record->templateInfo->setId != TemplateInfo::IpfixTemplate) 
		&& (record->templateInfo->setId != TemplateInfo::IpfixDataTemplate)) {
		record->removeReference();
		return;
	}
	
	
	// convert ipfixrecord to connection struct
	Connection conn(record);
	conn.swapIfNeeded();

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
 * Registers timeout for function onTimeout in Timer
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
 * Calculates criterias for every host which were active during the ongoing interval
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
			double sum = 0;
			double qsum = 0;
			double variance;
			iter->second.succBiFlowStarts.sort();
			list<uint64_t>::iterator ptr1 = iter->second.succBiFlowStarts.begin();
			list<uint64_t>::iterator ptr2 = ptr1++;

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
			double sum = 0;
			double qsum = 0;
			double variance;
			iter->second.failedBiFlowStarts.sort();
			list<uint64_t>::iterator ptr1 = iter->second.failedBiFlowStarts.begin();
			list<uint64_t>::iterator ptr2 = ptr1++;

			for(; ptr1 != iter->second.failedBiFlowStarts.end(); ptr1++, ptr2++){
				sum += *ptr1 - *ptr2;
				qsum += (*ptr1 - *ptr2) * (*ptr1 - *ptr2);
			}
			//sample variance (stichprobenvarianz) /  n = failedBiFlowStarts.size()-1: the differences not the starting points itself
			variance = (1.0/(iter->second.failedBiFlowStarts.size()-2))*(qsum - ((1.0/(iter->second.failedBiFlowStarts.size()-1))*(sum*sum)));
			tcpFailedVariance = sqrt(variance)/(iter->second.failedBiFlowStarts.size()-1);
		}

		//decide whether researched host is peer-to-peer or not
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
		if((tcpVariance <= tcpVarianceThreshold) && (tcpVariance >= 0))
			points++;
		if(failedConsPercent > failedConsPercentThreshold)
			points++;
		if(tcpFailedRate > tcpFailedRateThreshold)
			points++;
		if((tcpFailedVariance <= tcpFailedVarianceThreshold) && (tcpFailedVariance >= 0))
			points++;

		//host is a p2p client
		if(points > 6){
			//send Message
			msg(MSG_INFO, "P2P client detected:");
			msg(MSG_INFO, "IP: %s, dstSubnet: %s, dstSubMask: %s", IPToString(iter->first).c_str(),
				IPToString(subnet).c_str(), IPToString(subnetmask).c_str());

			IDMEFMessage* msg = idmefManager.getNewInstance();
			msg->init(idmefTemplate, analyzerId);

			msg->setVariable("UDP_RATE", udpRate);
			(udpRate > udpRateThreshold) ? msg->setVariable("TRUE1", "1") : msg->setVariable("TRUE1", "0");

			msg->setVariable("UDP_HOST_RATE", udpHostRate);
			(udpHostRate > udpHostRateThreshold) ? msg->setVariable("TRUE2", "1") : msg->setVariable("TRUE2", "0");

			msg->setVariable("TCP_RATE",  tcpRate);
			(tcpRate > tcpRateThreshold) ? msg->setVariable("TRUE3", "1") : msg->setVariable("TRUE3", "0");

			msg->setVariable("COEXISTENT_TCP_CONS", coexistentTCPCons);
			(coexistentTCPCons > coexistentTCPConsThreshold) ? msg->setVariable("TRUE4", "1") : msg->setVariable("TRUE4", "0");

			msg->setVariable("RATE_LONG_TCP_CONS", rateLongTCPCons);
			(rateLongTCPCons > rateLongTCPConsThreshold) ? msg->setVariable("TRUE5", "1") : msg->setVariable("TRUE5", "0");

			msg->setVariable("TCP_VARIANCE", tcpVariance);
			((tcpVariance <= tcpVarianceThreshold) && (tcpVariance >= 0)) ? msg->setVariable("TRUE6", "1") : msg->setVariable("TRUE6", "0");

			msg->setVariable("FAILED_CONS_PERCENT", failedConsPercent);
			(failedConsPercent > failedConsPercentThreshold) ? msg->setVariable("TRUE7", "1") : msg->setVariable("TRUE7", "0");

			msg->setVariable("TCP_FAILED_RATE", tcpFailedRate);
			(tcpFailedRate > tcpFailedRateThreshold) ? msg->setVariable("TRUE8","1") : msg->setVariable("TRUE8", "0");

			msg->setVariable("TCP_FAILED_VARIANCE", tcpFailedVariance);
			((tcpFailedVariance <= tcpFailedVarianceThreshold) && (tcpFailedVariance >= 0)) ? msg->setVariable("TRUE9","1") : msg->setVariable("TRUE9", "0");

			msg->setVariable("PEER_ADDRESS", IPToString(iter->first).c_str());
			msg->applyVariables();
			send(msg);
		}

	}

	//new interval starts
	hostList.clear();
}

/**
 * Things to be done when the module is shutdown
 */
void P2PDetector::performShutdown()
{
	onTimeout(NULL);
}

/**
 * Decides whether a connection was successfull or only a connection attempt
 */
bool P2PDetector::succConn(Connection& conn){

	//packets must be send in both directions
	if((conn.srcPackets != 0) && (conn.dstPackets != 0)){
		//more data were transmitted then just header informations
		if(((ntohll(conn.srcOctets) / ntohll(conn.srcPackets)) > 40) || ((ntohll(conn.dstOctets) / ntohll(conn.dstPackets)) > 40)){
			//full 3-way-handshake exists and src aborted the connection abrupt
			if((ntohll(conn.dstPackets) > 1) && (ntohll(conn.srcPackets) > 2) &&
				((conn.dstTcpControlBits & (Connection::SYN | Connection::ACK)) == (Connection::SYN | Connection::ACK)) &&
				((conn.srcTcpControlBits & (Connection::SYN | Connection::ACK | Connection::RST)) == (Connection::SYN | Connection::ACK | Connection::RST)))
					return true;

			//full 3-way-handshake eixts and dst aborted the connection abrupt
			if((ntohll(conn.dstPackets) > 2) && (ntohll(conn.srcPackets) > 1) &&
				((conn.dstTcpControlBits & (Connection::SYN | Connection::ACK | Connection::RST)) == (Connection::SYN | Connection::ACK | Connection::RST)) &&
				((conn.srcTcpControlBits & (Connection::SYN | Connection::ACK)) == (Connection::SYN | Connection::ACK)))
					return true;

			//complete established and regularly closed connection from both sides
			if(((conn.dstTcpControlBits & (Connection::SYN | Connection::ACK | Connection::FIN)) == (Connection::SYN | Connection::ACK | Connection::FIN)) &&
				((conn.srcTcpControlBits & (Connection::SYN | Connection::ACK | Connection::FIN)) == (Connection::SYN | Connection::ACK | Connection::FIN)) &&
				(((ntohll(conn.dstPackets) > 2) && (ntohll(conn.srcPackets) > 3)) || ((ntohll(conn.dstPackets) > 3) && (ntohll(conn.srcPackets) > 2))))
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
					return true;

		}
	}
	return false;
}
