/*
 * VERMONT 
 * Copyright (C) 2008 David Eckhoff <sidaeckh@informatik.stud.uni-erlangen.de>
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

#include "RBSWormDetector.h"
#include "crc.hpp"
#include "common/Misc.h"

#include <arpa/inet.h>
#include <math.h>
#include <iostream>

#define P_F 0.00001 // probability of false alarm
#define P_D 0.99 // probability of worm detection

const char* RBSWormDetector::PAR_FAN_OUT = "FAN_OUT"; 
const char* RBSWormDetector::PAR_TOTALTIME = "TOTALTIME"; 


InstanceManager<IDMEFMessage> RBSWormDetector::idmefManager("IDMEFMessage");

/**
 * attention: parameter idmefexporter must be free'd by the creating instance, TRWPortWormDetector
 * does not dare to delete it, in case it's used
 */
RBSWormDetector::RBSWormDetector(uint32_t hashbits, uint32_t texppend, 
		uint32_t texpworm, uint32_t texpben, uint32_t tadaptint,uint32_t tcleanupint, float lambdaratio,
		string analyzerid, string idmeftemplate)
: hashBits(hashbits),
	timeExpirePending(texppend),
	timeExpireWorm(texpworm),
	timeExpireBenign(texpben),
	timeAdaptInterval(tadaptint),
	timeCleanupInterval(tcleanupint),
	lambda_ratio(lambdaratio),
	analyzerId(analyzerid),
	idmefTemplate(idmeftemplate)
{
	// make some initialization calculations
	hashSize = 1<<hashBits;
	lambda_0 = 0;
	lambda_1 = 0;


	/* caution: usually the lambda values are calculated after timeAdaptInterval but you can preset them */
	//lambda_0 = 3.5; // fanout frequency of a benign host
	//lambda_1 = lambda_ratio * lambda_0; // fanout frequency of a infected host


	float logeta_1 = logf(P_D/P_F);
	float logeta_0 = logf(1-P_D);

	if (lambda_0 && lambda_1) 
	{
		float temp_z = logf(lambda_1/lambda_0);
		float temp_n = lambda_1 - lambda_0;
		slope_0a = temp_z/temp_n; 
		slope_0b = logeta_0/temp_n;
		slope_1a = temp_z/temp_n;
		slope_1b = logeta_1/temp_n;
	}

	lastAdaption = time(0);
	lastCleanup = time(0);

	rbsEntries = new list<RBSEntry*>[hashSize];
	msg(MSG_INFO,"RBSWormDetector started");
	msg(MSG_INFO,"Initial values: lambdas %f %f, slopes %f - %f, slopes %f - %f, adaptinterval: %d ,cleaninterval: %d",lambda_0,lambda_1,slope_0a,slope_0b,slope_1a,slope_1b,timeAdaptInterval,timeCleanupInterval);
}

RBSWormDetector::~RBSWormDetector()
{
	delete[] rbsEntries;
}

void RBSWormDetector::onDataDataRecord(IpfixDataDataRecord* record)
{
	// convert ipfixrecord to connection struct
	Connection conn(record);

	// only use this connection if it was a connection attempt
	if (conn.srcTcpControlBits&Connection::SYN ) {
	//	msg(MSG_INFO,"NEW CONN: %x",conn.dstTcpControlBits);
		addConnection(&conn);
	}
	
}


void RBSWormDetector::addConnection(Connection* conn)
{
	RBSEntry* te = getEntry(conn);
	
	//we are still in the startup phase, dont do anything
	if (lambda_0 == 0) return;

	// this host was already decided on, don't do anything any more
	if (te->decision != PENDING) return;


	te->timeExpire = time(0) + timeExpirePending;

	// only work with this connection, if it wasn't accessed earlier by this host
	if (find(te->accessedHosts.begin(), te->accessedHosts.end(), conn->dstIP) != te->accessedHosts.end()) return;

	te->accessedHosts.push_back(conn->dstIP);

	te->timeExpire = time(0) + timeExpirePending;


	// aggregate new connection into entry
	te->numFanouts++;

	// calculate thresholds

	float thresh_0 = te->numFanouts * slope_0a - slope_0b;
	float thresh_1 = te->numFanouts * slope_1a - slope_1b;
	uint32_t time_ela = time(0) - te->startTime;

	msg(MSG_INFO,"Thresholds calculated: H1: %f H0: %f TIME: %d CONNS: %d",thresh_0, thresh_1,time_ela,te->numFanouts);
	msg(MSG_INFO, "dstIP: %s", IPToString(conn->dstIP).c_str());
	msg(MSG_INFO, "srcIP: %s", IPToString(te->srcIP).c_str());

	if (te->numFanouts < lambda_0) return;  //need more connections to evaluate

	// look if information is adequate for deciding on host
	if (time_ela>thresh_0)
	{
		// no worm, just let entry stay here until it expires
		te->timeExpire = time(0)+timeExpireBenign;
		te->decision = BENIGN;
	}
	else if (time_ela<thresh_1) 
	{
		//this is a worm
		te->decision = WORM;
		statNumWorms++;
		te->timeExpire = time(0)+timeExpireWorm;
		msg(MSG_DEBUG, "Worm detected:");
		msg(MSG_DEBUG, "srcIP: %s", IPToString(te->srcIP).c_str());
		msg(MSG_DEBUG, "numFanOut: %d, totalTime: %d",te->numFanouts, time_ela);

		IDMEFMessage* msg = idmefManager.getNewInstance();
		msg->init(idmefTemplate, analyzerId);
		msg->setVariable(PAR_FAN_OUT, te->numFanouts);
		msg->setVariable(PAR_TOTALTIME, (int) time_ela);
		msg->setVariable(IDMEFMessage::PAR_SOURCE_ADDRESS, IPToString(te->srcIP));
		msg->applyVariables();
		send(msg);
	}
}

/**
 * returns entry in hashtable for the given connection
 * if it was not found, a new entry is created and returned
 */
RBSWormDetector::RBSEntry* RBSWormDetector::getEntry(Connection* conn)
{
	time_t curtime = time(0);
	uint32_t hash = crc32(0, 4, reinterpret_cast<char*>(&conn->srcIP)) & (hashSize-1);

	//regularly adapt new values
	if (lastAdaption+timeAdaptInterval < (uint32_t) curtime) 
	{
		adaptFrequencies();
		lastAdaption = curtime;
	}

	// regularly cleanup expired entries in hashtable
	if (lastCleanup+timeCleanupInterval < (uint32_t)curtime) {
		cleanupEntries();
		lastCleanup = curtime;
	}

	list<RBSEntry*>::iterator iter = rbsEntries[hash].begin();
	while (iter != rbsEntries[hash].end()) {
		if ((*iter)->srcIP == conn->srcIP) {
			// found the entry
			return *iter;
		}
		iter++;
	}

	// no entry found, create a new one
	RBSEntry* rbs = createEntry(conn);
	rbsEntries[hash].push_back(rbs);

	return rbs;
}

/**
 * creates a new rbs entry and sets status to pending
 */

RBSWormDetector::RBSEntry* RBSWormDetector::createEntry(Connection* conn)
{
	RBSEntry* rbs = new RBSEntry();
	rbs->srcIP = conn->srcIP;
	rbs->numFanouts = 0;
	rbs->startTime = time(0);
	rbs->timeExpire = time(0) + timeExpirePending;
	rbs->decision = PENDING;
	statEntriesAdded++;
	msg(MSG_INFO,"New RBSEntry created");
	return rbs;
}

/**
 * erases entries in our hashtable which are expired
 */	
void RBSWormDetector::cleanupEntries()
{
	time_t curtime = time(0);

	for (uint32_t i=0; i<hashSize; i++) {
		if (rbsEntries[i].size()==0) continue;

		list<RBSEntry*>::iterator iter = rbsEntries[i].begin();
		while (iter != rbsEntries[i].end()) {
			if (curtime > (*iter)->timeExpire) {
				RBSEntry* te = *iter;
				iter = rbsEntries[i].erase(iter);
				delete te;
				statEntriesRemoved++;
			} else {
				iter++;
			}
		}
	}
}



/**
 * adapt new benign/worm frequencies based on stored data
 */
void RBSWormDetector::adaptFrequencies () 
{
	time_t curtime = time(0);
	uint32_t count = 0;	
	uint32_t temp1 = 0;


	list<RBSEntry*> adaptList;	

	//put all entries in one list to calculate trimmed mean
	for (uint32_t i=0; i<hashSize; i++) {
		if (rbsEntries[i].size()==0) continue;

		list<RBSEntry*>::iterator iter = rbsEntries[i].begin();

		while (iter != rbsEntries[i].end()) 
		{
			adaptList.push_back(*iter);
			iter++;	
		}
	}

	//sort list to cut off top and bottom 10 percent
	adaptList.sort(RBSWormDetector::comp_entries);

	uint32_t num10 = adaptList.size()/10;

	list<RBSEntry*>::iterator iter = adaptList.begin();


	while (iter != adaptList.end()) 
	{
		count++;
		//trim bottom and top 10%
		if (count > num10 && count <( adaptList.size() - num10)) 
			temp1 += (*iter)->numFanouts/((*iter)->startTime-curtime);

		iter++;
	}

	if (temp1 < 3) {
		msg(MSG_DEBUG,"too little traffic");
		return;
	}

	lambda_0 = temp1;		
	lambda_1 = lambda_ratio*lambda_0;

	float logeta_1 = logf(P_D/P_F);
	float logeta_0 = logf(1-P_D);
	float temp_z = logf(lambda_1/lambda_0);
	float temp_n = lambda_1 - lambda_0;
	slope_0a = temp_z/temp_n;
	slope_0b = logeta_0/temp_n;
	slope_1a = temp_z/temp_n;
	slope_1b = logeta_1/temp_n;

	msg(MSG_INFO,"Adapted Frequencies, lambda_0=%f",lambda_0);
}

/*
 * compare fuction to sort entry list
 * 
 */
bool RBSWormDetector::comp_entries(RBSEntry* a,RBSEntry* b) {
	uint32_t curtime = time(0);
	return (a->numFanouts/(a->startTime - curtime)) <  (b->numFanouts/(b->startTime - curtime));
}

string RBSWormDetector::getStatistics()
{
	ostringstream oss;
	oss << "rbsworm: ips cached       : " << statEntriesAdded-statEntriesRemoved << endl;
	oss << "rbsworm: ips removed      : " << statEntriesRemoved << endl;
	oss << "rbsworm: worms detected: " << statNumWorms << endl;
	return oss.str();
}

std::string RBSWormDetector::getStatisticsXML()
{
	ostringstream oss;
	oss << "<rbswormdetector>" << endl;
	oss << "<frequencies>" << lambda_0 << " " << lambda_1 << "</frequencies>" << endl;
	oss << "<entrycount>" << statEntriesAdded-statEntriesRemoved << "</entrycount>" << endl;
	oss << "<wormsdetected>" << statNumWorms << "</wormsdetected>" << endl;
	oss << "</rbswormdetector>" << endl;
	return oss.str();
}


