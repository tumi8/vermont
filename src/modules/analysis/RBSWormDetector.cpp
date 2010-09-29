
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
#include "common/crc.hpp"
#include "common/Misc.h"

#include <arpa/inet.h>
#include <math.h>
#include <iostream>

#define P_F 0.00001 // probability of false alarm
#define P_D 0.99 // probability of worm detection

const char* RBSWormDetector::PAR_FAN_OUT = "FAN_OUT"; 
const char* RBSWormDetector::PAR_TOTALTIME = "TOTALTIME"; 
const char* RBSWormDetector::PAR_HOSTS = "HOSTS"; 


InstanceManager<IDMEFMessage> RBSWormDetector::idmefManager("RBSWormIDMEFMessage", 0);

/**
 * attention: parameter idmefexporter must be free'd by the creating instance, TRWPortWormDetector
 * does not dare to delete it, in case it's used
 */
RBSWormDetector::RBSWormDetector(uint32_t hashbits, uint32_t texppend, 
		uint32_t texpworm, uint32_t texpben, uint32_t tadaptint,uint32_t tcleanupint, float lambdaratio,
		string analyzerid, string idmeftemplate,map<uint32_t,uint32_t> subNets)
: hashBits(hashbits),
	timeExpirePending(texppend),
	timeExpireWorm(texpworm),
	timeExpireBenign(texpben),	
	timeCleanupInterval(tcleanupint),
	timeAdaptInterval(tadaptint),	
	analyzerId(analyzerid),
	idmefTemplate(idmeftemplate),
	lambda_ratio(lambdaratio),	
	subnets(subNets)
{
	// make some initialization calculations
	hashSize = 1<<hashBits;
	lambda_0 = 0;


	/* caution: usually the lambda values are calculated after timeAdaptInterval but you can preset them */
	//	lambda_0 = 3.5; // fanout frequency of a benign host
	lambda_1 = lambda_ratio * lambda_0; // fanout frequency of a infected host


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
	statEntriesAdded = 0;
	statEntriesRemoved = 0;
	statNumWorms = 0;
	statCurBenign = 0;

	rbsEntries = new list<RBSEntry*>[hashSize];
	msg(MSG_INFO,"RBSWormDetector started");
}
/*
 * Destructor frees memory
 */
RBSWormDetector::~RBSWormDetector()
{
	delete[] rbsEntries;
}

void RBSWormDetector::onDataRecord(IpfixDataRecord* record)
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
	// only use this connection if it was a connection attempt
	if (conn.srcTcpControlBits&Connection::SYN ) {

		bool valid = false;

		map<uint32_t,uint32_t>::iterator iter = subnets.begin();
		while (iter != subnets.end()) 
		{
		if ((conn.srcIP & iter->second) == iter->first) 
		{
			valid=true;
			break;
		}
		iter++;
		}
		if (valid) addConnection(&conn);

	}
	record->removeReference();
}


void RBSWormDetector::addConnection(Connection* conn)
{
	RBSEntry* te = getEntry(conn);
	
	//worms must not influence our average fanout frequency of non-worm hosts
	if (te->decision == WORM) return;
	
	// FOLLOWING CODE IS FOR BENIGN AND PENDING HOSTS

	// only work with this connection, if it wasn't accessed earlier by this host
	if (find(te->accessedHosts.begin(), te->accessedHosts.end(), conn->dstIP) != te->accessedHosts.end()) return;
	te->accessedHosts.push_back(conn->dstIP);

	//host was moved from benign to pending (for average fanouts)
	if (te->switched)	
	{
	te->switched = false;
	te->startTime = conn->srcTimeStart;
	}

	//timeelams represents time since 1970 in milliseconds
	uint64_t time_elams = conn->srcTimeStart; 
	

	//duration between last 2 packets
	uint64_t intarrival = labs((int64_t) (time_elams - te->lastPacket));

	//last two connection attempts where within 1 second
	if (intarrival < 1000) 
	{	
		te->totalSSNum++;
		te->totalSSDur += intarrival;
		te->mean = (te->totalSSDur/ (double) 1000) / (double) (te->totalSSNum);
	}

	te->lastPacket = time_elams;

	//we are still in the startup phase, dont decide on hosts
	if (lambda_0 == 0) return;

	// this host was already decided on, don't do anything any more
	if (te->decision != PENDING) return;


	// FOLLOWING CODE IS FOR PENDING HOSTS ONLY


	te->timeExpire = time(0) + timeExpirePending;
	te->numFanouts++;
	

	double trace_ela = (double) (time_elams - te->startTime) / 1000;
	//traceela is packet trace time in seconds

	// calculate thresholds
	float thresh_0 = te->numFanouts * slope_0a - slope_0b;
	float thresh_1 = te->numFanouts * slope_1a - slope_1b;

	//need more connections to evaluate
	if (te->numFanouts < lambda_0) return;  

	// look if information is adequate for deciding on host
	if (trace_ela>thresh_0)
	{
		// no worm, just let entry stay here until it expires
		te->timeExpire = time(0)+timeExpireBenign;
		te->decision = BENIGN;
		statCurBenign++;
	}
	else if (trace_ela<thresh_1) 
	{
		//this is a worm
		te->decision = WORM;
		statNumWorms++;
		te->timeExpire = time(0)+timeExpireWorm;
		msg(MSG_DEBUG, "Worm detected:");
		msg(MSG_DEBUG, "srcIP: %s", IPToString(te->srcIP).c_str());
		msg(MSG_DEBUG, "numFanOut: %d, totalTime: %d",te->numFanouts, trace_ela);

		IDMEFMessage* msg = idmefManager.getNewInstance();
		msg->init(idmefTemplate, analyzerId);
		msg->setVariable(PAR_FAN_OUT, (uint32_t) te->numFanouts);
		msg->setVariable(PAR_TOTALTIME, trace_ela);
		string hosts;
		list<uint32_t>::iterator iter = te->accessedHosts.begin();
		while(iter != te->accessedHosts.end()) 
		{
		hosts.append(IPToString(*iter));
		hosts.append(" ");
		iter++;
		}	
		msg->setVariable(PAR_HOSTS,hosts.c_str());
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
		lastAdaption = curtime;
		adaptFrequencies();
		
	}

	// regularly cleanup expired entries in hashtable
	if (lastCleanup+timeCleanupInterval < (uint32_t)curtime) {
		lastCleanup = curtime;
		cleanupEntries();		
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
	rbs->totalSSDur = 0;
	rbs->lastPacket = 0;
	rbs->startTime = conn->srcTimeStart;
	rbs->totalSSNum = 0;
	rbs->timeExpire = time(0) + timeExpirePending;
	rbs->decision = PENDING;
	rbs->mean = 0;
	statEntriesAdded++;
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
				//BENIGN Host are not erased but moved to pending state with all stats reset except the subsecond interarrival times
				if ((*iter)->decision == BENIGN)
				{
				te->decision = PENDING;
				te->switched = true;
				te->numFanouts = 0;
				te->timeExpire = time(0) + timeExpirePending;
				te->accessedHosts.clear();
				statCurBenign--;
				iter++;
				}
				else
				{
				//Host can be cleaned
				iter = rbsEntries[i].erase(iter);
				delete te;
				statEntriesRemoved++;
				}
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
	uint32_t count = 0;	
	double temp1 = 0;

	list<RBSEntry*> adaptList;	
	bool first = false;
	if (lambda_0 == 0) first = true;

	//put all entries in one list to calculate trimmed mean
	for (uint32_t i=0; i<hashSize; i++) {
		if (rbsEntries[i].size()==0) continue;

		list<RBSEntry*>::iterator iter = rbsEntries[i].begin();

		while (iter != rbsEntries[i].end()) 
		{
			if ((*iter)->mean != 0 && (*iter)->decision != WORM)
			{
			adaptList.push_back(*iter);
			}
			iter++;	
		}
	}
	//sort list to cut off top and bottom 10 percent
	adaptList.sort(RBSWormDetector::comp_entries);	

	msg(MSG_FATAL,"meta list size %d",adaptList.size());
	uint32_t num10 = adaptList.size()/10;

	list<RBSEntry*>::iterator iter = adaptList.begin();

	uint32_t valid = 0;
	while (iter != adaptList.end()) 
	{
		count++;
		//trim bottom and top 10%
		if (count > num10 && count <( adaptList.size() - num10)) 
		{
				valid++;
				temp1 += (*iter)->mean;
		}
		//reset all data. we need a clean start every 10 minutes.
		(*iter)->mean = 0;
		(*iter)->totalSSDur = 0;
		(*iter)->totalSSNum = 0;
		(*iter)->lastPacket = 0;
		(*iter)->accessedHosts.clear();
		iter++;
	}

	if (valid>0 && temp1>0)
	lambda_0 = 1.0 / (temp1/valid);	
	lambda_1 = lambda_ratio*lambda_0;


	//recalculate slopes of threshold lines
	if (lambda_0)
		{
	float logeta_1 = logf(P_D/P_F);
	float logeta_0 = logf(1-P_D);
	float temp_z = logf(lambda_1/lambda_0);
	float temp_n = lambda_1 - lambda_0;
	slope_0a = temp_z/temp_n;
	slope_0b = logeta_0/temp_n;
	slope_1a = temp_z/temp_n;
	slope_1b = logeta_1/temp_n;
	msg(MSG_FATAL,"Adapted Frequencies, lambda_0=%f with hosts=%d",lambda_0,valid++);
		}
	else
		{
	msg(MSG_ERROR,"Too little traffic for adaption");
		}
	
	if (!first) return;

	//after the first adaption all hosts are cleared no matter what.
	for (uint32_t i=0; i<hashSize; i++) 
	{
		if (rbsEntries[i].size()==0) continue;

		list<RBSEntry*>::iterator iter = rbsEntries[i].begin();

		while (iter != rbsEntries[i].end()) 
		{
			RBSEntry* te = *iter;
			iter=rbsEntries[i].erase(iter);
			delete te;
			statEntriesRemoved++;
		}
	}
}

/*
 * compare fuction to sort entry list
 * 
 */
bool RBSWormDetector::comp_entries(RBSEntry* a,RBSEntry* b) {
	return a->mean < b->mean;
}

string RBSWormDetector::getStatistics()
{
	ostringstream oss;
	oss << "rbsworm: ips cached       : " << statEntriesAdded-statEntriesRemoved << endl;
	oss << "rbsworm: ips removed      : " << statEntriesRemoved << endl;
	oss << "rbsworm: worms detected: " << statNumWorms << endl;
	return oss.str();
}

std::string RBSWormDetector::getStatisticsXML(double)
{
	ostringstream oss;
	oss << "<rbswormdetector>" << endl;
	oss << "<frequencies>" << lambda_0 << " " << lambda_1 << "</frequencies>" << endl;
	oss << "<entrycount>" << statEntriesAdded-statEntriesRemoved << "</entrycount>" << endl;

	oss << "<currentBenign>" << statCurBenign << "</currentBenign>" << endl;
	oss << "<wormsdetected>" << statNumWorms << "</wormsdetected>" << endl;
	oss << "<nextadaptionin>" << timeAdaptInterval - (time(0) - lastAdaption)   << "</nextadaptionin>" << endl;
	oss << "<nextcleanup>" << timeCleanupInterval - (time(0) - lastCleanup)   << "</nextcleanup>" << endl;
	oss << "</rbswormdetector>" << endl;
	return oss.str();
}


