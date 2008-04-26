
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

#include "AutoFocus.h"
#include "crc.hpp"
#include "common/Misc.h"

#include <arpa/inet.h>
#include <math.h>
#include <iostream>


InstanceManager<IDMEFMessage> AutoFocus::idmefManager("IDMEFMessage");

/**
 * attention: parameter idmefexporter must be free'd by the creating instance, TRWPortWormDetector
 * does not dare to delete it, in case it's used
 */
	AutoFocus::AutoFocus(uint32_t hashbits, uint32_t ttreeint, uint32_t nummaxr, string analyzerid, string idmeftemplate)
: hashBits(hashbits),
	timeTreeInterval(ttreeint),
	numMaxResults(nummaxr),
	analyzerId(analyzerid),
	idmefTemplate(idmeftemplate)

{
	hashSize = 1<<hashBits;
	lastTreeBuilt = time(0);
	statEntriesAdded = 0;
	totalData = 0;
	listIPRecords = new list<IPRecord*>[hashSize];
	msg(MSG_INFO,"AutoFocus started");
}

AutoFocus::~AutoFocus()
{
	delete[] listIPRecords;
}

void AutoFocus::onDataDataRecord(IpfixDataDataRecord* record)
{
	// convert ipfixrecord to connection struct
	Connection conn(record);

	
	uint32_t subnet = ntohl(2210136064UL);
	uint32_t mask = ntohl(4294901760UL);

	
	if ((conn.srcIP & mask) != subnet) 
	{
	conn.srcIP = conn.dstIP;
	}

	if ((conn.srcIP & mask) == subnet) 
	{
	addConnection(&conn);
	}

	record->removeReference();
}


void AutoFocus::addConnection(Connection* conn)
{
	IPRecord* te = getEntry(conn);
	
	uint64_t deltacount = ntohll(conn->srcOctets) + ntohll(conn->dstOctets);

	te->payload += deltacount;
	totalData += deltacount;

	//if (te->payload != 0) msg(MSG_FATAL,"new payload: %llu",te->payload);
}

/**
 * returns entry in hashtable for the given connection
 * if it was not found, a new entry is created and returned
 */
AutoFocus::IPRecord* AutoFocus::getEntry(Connection* conn)
{
	time_t curtime = time(0);
	uint32_t hash = crc32(0, 4, reinterpret_cast<char*>(&conn->srcIP)) & (hashSize-1);

	
	if (lastTreeBuilt+timeTreeInterval < (uint32_t) curtime) 
	{
		lastTreeBuilt = curtime;
		buildTree();

	}


	list<IPRecord*>::iterator iter = listIPRecords[hash].begin();
	while (iter != listIPRecords[hash].end()) {
		if ((*iter)->subnetIP == conn->srcIP) {
			// found the entry
			return *iter;
		}
		iter++;
	}

	// no entry found, create a new one
	IPRecord* te = createEntry(conn);
	listIPRecords[hash].push_back(te);

	return te;
}

/**
 * creates a new iprecord and sets status to pending
 */

AutoFocus::IPRecord* AutoFocus::createEntry(Connection* conn)
{
	IPRecord* te = new IPRecord();
	te->subnetIP = conn->srcIP;
	te->subnetBits = 32;
	te->payload = 0;
	statEntriesAdded++;
	return te;
}

uint32_t AutoFocus::distance(treeNode* a,treeNode* b) 
{
	return ntohl(a->data.subnetIP) ^ ntohl(b->data.subnetIP);
}
/**
 * adapt new benign/worm frequencies based on stored data
 */
void AutoFocus::buildTree () 
{
	treeNode* root;
	list<treeNode*> tree;

	//msg(MSG_FATAL,"Values %lu %u",totalData,numMaxResults);

	uint64_t threshold = (uint64_t) (totalData / numMaxResults);

//	msg(MSG_FATAL,"THRESHOLD %lu",threshold);
//	msg(MSG_FATAL,"MAKING SUPERLIST");

	for (uint32_t i=0; i<hashSize; i++) {
		if (listIPRecords[i].size()==0) continue;

		list<IPRecord*>::iterator iter = listIPRecords[i].begin();

		while (iter != listIPRecords[i].end()) 
		{
			treeNode* entry = new treeNode;
			entry->data.subnetIP = (*iter)->subnetIP;
			entry->data.subnetBits = (*iter)->subnetBits;
			entry->data.payload = (*iter)->payload;
			entry->left = NULL;
			entry->right = NULL;
			entry->delta = (*iter)->payload;
	//		msg(MSG_FATAL,"%lu",entry->delta);
			tree.push_back(entry);
			iter++;	
		}
	}

	msg(MSG_FATAL,"LIST SORTED");
	tree.sort(AutoFocus::comp_entries);

	//* BUILD TREE *//

	list<treeNode*>::iterator iter = tree.begin();
	int count = 0;

	while (iter != tree.end())
	{


		if ((*iter)->delta > threshold) 
		{
			uint64_t temp = (*iter)->delta * 100;
			double part =  (double) temp /  (double) totalData;
			
			msg(MSG_FATAL,"IP %s/%d",IPToString((*iter)->data.subnetIP).c_str(),(*iter)->data.subnetBits);
			msg(MSG_FATAL,"Total Traffic %llu, Delta Traffic %llu, Percentage of total traffic %f",(*iter)->data.payload,(*iter)->delta,part);
		
			(*iter)->delta = 0;
		}


		if (*iter == tree.back()) { iter++; continue; }

		uint32_t a = distance(*iter,*(iter++));
		uint32_t b;
		count++;

		if (*iter != tree.back())
		{
			b=distance(*iter,*(iter++));
			iter--;		
		}
		else
		{
			b = (uint32_t) pow(2,32)-1;
		}
		count++;
		iter--;

		if (a<=b)
		{

			treeNode* newnode = new treeNode;

			tree.insert(iter,newnode);
			newnode->left = *iter;
			uint32_t ip1 = ntohl(((*iter)->data).subnetIP);
			uint64_t d1 = (*iter)->delta;
			uint64_t p1 = (*iter)->data.payload; 
			
			iter = tree.erase(iter);
			newnode->right = *iter;

			uint32_t ip2 = ntohl(((*iter)->data).subnetIP);
			uint64_t d2 = (*iter)->delta;
			uint64_t p2 = (*iter)->data.payload; 

			newnode->delta = d1+d2;
			//msg(MSG_FATAL,"%lu %lu %lu",newnode->delta,d1,d2);
			newnode->data.payload = p1+p2;

			iter = tree.erase(iter);

			int subbits = (uint32_t) (round(log(a)/log(2)+0.5));

			newnode->data.subnetIP = htonl((ip1>>subbits)<<subbits);
			newnode->data.subnetBits = 32 - subbits;
			iter--;
			if (*iter != tree.front()) iter--;
		}
		else
			iter++;



	}

}

/*
 * compare fuction to sort entry list
 * 
 */
bool AutoFocus::comp_entries(treeNode* a,treeNode* b) {
	return ntohl(a->data.subnetIP) < ntohl(b->data.subnetIP);
}

string AutoFocus::getStatistics()
{
	ostringstream oss;
	return oss.str();
}

std::string AutoFocus::getStatisticsXML()
{
	ostringstream oss;
	oss << "<AutoFocus>" << endl;
	oss << "<entrycount>" << statEntriesAdded  << "</entrycount>" << endl;

	oss << "<nexttreein>" << timeTreeInterval - (time(0) - lastTreeBuilt)   << "</nexttreein>" << endl;
	oss << "</AutoFocus>" << endl;
	return oss.str();
}


