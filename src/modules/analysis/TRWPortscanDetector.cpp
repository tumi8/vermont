/*
 * VERMONT 
 * Copyright (C) 2007 Tobias Limmer <tobias.limmer@informatik.uni-erlangen.de>
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

#include "TRWPortscanDetector.h"
#include "common/crc.hpp"
#include "common/Misc.h"

#include <arpa/inet.h>
#include <math.h>
#include <iostream>


const char* TRWPortscanDetector::PAR_SUCC_CONNS = "SUCC_CONNS";
const char* TRWPortscanDetector::PAR_FAILED_CONNS = "FAILED_CONNS";


InstanceManager<IDMEFMessage> TRWPortscanDetector::idmefManager("TRWPortscanIDMEFMessage", 0);

/**
 * attention: parameter idmefexporter must be free'd by the creating instance, TRWPortscanDetector
 * does not dare to delete it, in case it's used
 */
TRWPortscanDetector::TRWPortscanDetector(uint32_t hashbits, uint32_t texppend, 
		uint32_t texpscan, uint32_t texpben, uint32_t tcleanint, 
		string analyzerid, string idmeftemplate)
	: hashBits(hashbits),
	  timeExpirePending(texppend),
	  timeExpireScanner(texpscan),
	  timeExpireBenign(texpben),
	  timeCleanupInterval(tcleanint),
	  analyzerId(analyzerid),
	  idmefTemplate(idmeftemplate)
{
	// make some initialization calculations
	hashSize = 1<<hashBits;
	float theta_0 = 0.8; // probability that benign host makes successful connection
	float theta_1 = 0.2; // probability that malicious host makes successful connection
	float P_F = 0.00001; // probability of false alarm
	float P_D = 0.99999; // probability of scanner detection

	float eta_1 = 1/P_F;
	float eta_0 = 1-P_D;
	logeta_0 = logf(eta_0);
	logeta_1 = logf(eta_1);
	X_0 = logf(theta_1/theta_0);
	X_1 = logf((1-theta_1)/(1-theta_0));
	msg(LOG_NOTICE, "TRW variables: logeta_0: %f, logeta_1: %f, X_0: %f, X_1: %f", logeta_0, logeta_1, X_0, X_1);
	lastCleanup = time(0);
	
	try {
		trwEntries = new list<TRWEntry*>[hashSize];
	} catch(std::bad_alloc& ba) {
		THROWEXCEPTION("Error allocating TRWEntries in TRWPortscanDetector. Try reducing the number of hashbits: %s", ba.what());
	}
}

TRWPortscanDetector::~TRWPortscanDetector()
{
	delete[] trwEntries;
}

void TRWPortscanDetector::onDataRecord(IpfixDataRecord* record)
{
	// only treat non-Options Data Records (although we cannot be sure that there is a Flow inside)
	if((record->templateInfo->setId != TemplateInfo::NetflowTemplate) 
		&& (record->templateInfo->setId != TemplateInfo::IpfixTemplate)) {
		record->removeReference();
		return;
	}
	
	// convert ipfixrecord to connection struct
	Connection conn(record);
	
	conn.swapIfNeeded();

	// only use this connection if it was a connection attempt
	if (conn.srcTcpControlBits&Connection::SYN) {
		addConnection(&conn);
	}

	record->removeReference();
}

TRWPortscanDetector::TRWEntry* TRWPortscanDetector::createEntry(Connection* conn)
{
	TRWEntry* trw = new TRWEntry();
	trw->srcIP = conn->srcIP;
	trw->dstSubnet = 0;
	trw->dstSubnetMask = 0xFFFFFFFF;
	trw->numFailedConns = 0;
	trw->numSuccConns = 0;
	trw->timeExpire = time(0) + timeExpirePending;
	trw->decision = PENDING;
	trw->S_N = 0;

	statEntriesAdded++;

	return trw;
}

/**
 * erases entries in our hashtable which are expired
 */
void TRWPortscanDetector::cleanupEntries()
{
	time_t curtime = time(0);

	for (uint32_t i=0; i<hashSize; i++) {
		if (trwEntries[i].size()==0) continue;

		list<TRWEntry*>::iterator iter = trwEntries[i].begin();
		while (iter != trwEntries[i].end()) {
			if (curtime > (*iter)->timeExpire) {
				TRWEntry* te = *iter;
				iter = trwEntries[i].erase(iter);
				delete te;
				statEntriesRemoved++;
			} else {
				iter++;
			}
		}
	}
}

/**
 * returns entry in hashtable for the given connection
 * if it was not found, a new entry is created and returned
 */
TRWPortscanDetector::TRWEntry* TRWPortscanDetector::getEntry(Connection* conn)
{
	time_t curtime = time(0);
	uint32_t hash = crc32(0, 2, &reinterpret_cast<char*>(&conn->srcIP)[2]) & (hashSize-1);

	// regularly cleanup expired entries in hashtable
	if (lastCleanup+timeCleanupInterval < (uint32_t)curtime) {
		cleanupEntries();
		lastCleanup = curtime;
	}

	list<TRWEntry*>::iterator iter = trwEntries[hash].begin();
	while (iter != trwEntries[hash].end()) {
		if ((*iter)->srcIP == conn->srcIP) {
			// found the entry
			return *iter;
		}
		iter++;
	}

	// no entry found, create a new one
	TRWEntry* trw = createEntry(conn);
	trwEntries[hash].push_back(trw);

	return trw;
}

void TRWPortscanDetector::addConnection(Connection* conn)
{
	TRWEntry* te = getEntry(conn);

	// this host was already decided on, don't do anything any more
	if (te->decision != PENDING) return;

	// determine if connection was a failed or successful connection attempt
	// by looking if answering host sets the syn+ack bits for the threeway handshake
	bool connsuccess;
	if ((conn->dstTcpControlBits&(Connection::SYN|Connection::ACK))!=(Connection::SYN|Connection::ACK)) {
		// no, this is not a successful connection attempt!
		te->numFailedConns++;
		connsuccess = false;

	} else {
		te->numSuccConns++;
		connsuccess = true;
	}

	te->timeExpire = time(0) + timeExpirePending;

	// only work with this connection, if it wasn't accessed earlier by this host
	if (find(te->accessedHosts.begin(), te->accessedHosts.end(), conn->dstIP) != te->accessedHosts.end()) return;

	te->accessedHosts.push_back(conn->dstIP);

	te->S_N += (connsuccess ? X_0 : X_1);

	// aggregate new connection into entry
	if (te->dstSubnet==0 && te->dstSubnetMask==0xFFFFFFFF) {
		te->dstSubnet = conn->dstIP;
	} else {
		// adapt subnet mask so that new destination ip is inside given subnet
		while ((te->dstSubnet&te->dstSubnetMask)!=(conn->dstIP&te->dstSubnetMask)) {
			te->dstSubnetMask = ntohl(te->dstSubnetMask);
			te->dstSubnetMask <<= 1;
			te->dstSubnetMask = htonl(te->dstSubnetMask);
			te->dstSubnet &= te->dstSubnetMask;
		}
	}

	DPRINTF_INFO("IP: %s, S_N: %f", IPToString(te->srcIP).c_str(), te->S_N);
	
	// look if information is adequate for deciding on host
	if (te->S_N<logeta_0) {
		// no portscanner, just let entry stay here until it expires
		te->timeExpire = time(0)+timeExpireBenign;
		te->decision = BENIGN;
	} else if (te->S_N>logeta_1) {
		//this is a portscanner!
		te->decision = SCANNER;
		statNumScanners++;
		te->timeExpire = time(0)+timeExpireScanner;
		msg(LOG_INFO, "portscanner detected:");
		msg(LOG_INFO, "srcIP: %s, dstSubnet: %s, dstSubMask: %s", IPToString(te->srcIP).c_str(), 
				IPToString(te->dstSubnet).c_str(), IPToString(te->dstSubnetMask).c_str());
		msg(LOG_INFO, "numFailedConns: %d, numSuccConns: %d", te->numFailedConns, te->numSuccConns);

		IDMEFMessage* msg = idmefManager.getNewInstance();
		msg->init(idmefTemplate, analyzerId);
		msg->setVariable(PAR_SUCC_CONNS, te->numSuccConns);
		msg->setVariable(PAR_FAILED_CONNS, te->numFailedConns);
		msg->setVariable(IDMEFMessage::PAR_SOURCE_ADDRESS, IPToString(te->srcIP));
		msg->setVariable(IDMEFMessage::PAR_TARGET_ADDRESS, IPToString(te->dstSubnet)+"/"+IPToString(te->dstSubnetMask));
		msg->applyVariables();
		send(msg);
	}
}

string TRWPortscanDetector::getStatisticsXML(double interval)
{
	ostringstream oss;
	oss << "<hostsCached>" << statEntriesAdded-statEntriesRemoved << "</hostsCached>";
	oss << "<totalHostsRemoved>" << statEntriesRemoved << "</totalHostsRemoved>";
	oss << "<totalScannersDetected>" << statNumScanners << "</totalScannersDetected>";
	return oss.str();
}


