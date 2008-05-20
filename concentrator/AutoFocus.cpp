
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
	AutoFocus::AutoFocus(uint32_t hashbits, uint32_t ttreeint, uint32_t nummaxr, uint32_t numtrees, string analyzerid, string idmeftemplate, logtype lgtype)
: hashBits(hashbits),
	timeTreeInterval(ttreeint),
	numMaxResults(nummaxr),
	numTrees(numtrees),
	analyzerId(analyzerid),
	idmefTemplate(idmeftemplate),
	m_treeRecords(numtrees)

{
	hashSize = 1<<hashBits;
	lastTreeBuilt = time(0);
	statEntriesAdded = 0;
	totalData = 0;

	m_treeCount = 0;
	m_treeRecords.clear();


	lg_type = lgtype;

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
	uint32_t fanout = 0;
	te->payload += deltacount;

	if ((conn->srcTcpControlBits&Connection::SYN) && (ntohll(conn->srcTimeStart)<ntohll(conn->dstTimeStart)))
	{
		fanout = 1;
		te->fanouts++;
	}

	switch (lg_type) {
		case AutoFocus::lg_payload: 
			{
				totalData += deltacount; 
				break;
			}
		case AutoFocus::lg_fanouts:
			{
				totalData += fanout;
				break;
			}
		default: break;
	}

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

void AutoFocus::deleteTree(treeNode* root) 
{

	if (root->left != NULL)
		deleteTree(root->left);

	if (root->right != NULL)
		deleteTree(root->right);

	delete root;
	return;
}

AutoFocus::IPRecord* AutoFocus::createEntry(Connection* conn)
{
	IPRecord* te = new IPRecord();
	te->subnetIP = conn->srcIP;
	te->subnetBits = 32;
	te->payload = 0;
	te->fanouts = 0;
	statEntriesAdded++;
	return te;
}


void AutoFocus::deleteRecord(int index)
{

	deleteTree(m_treeRecords[index]->root);	
	delete m_treeRecords[index];
}

AutoFocus::treeNode* AutoFocus::getComparismValue(treeNode* match,uint32_t index)
{
	if (m_treeRecords[index] == NULL) return NULL;
	else 
	{
		uint32_t sip = ntohl(match->data.subnetIP);
		uint32_t sbits = match->data.subnetBits;
		treeNode* current = m_treeRecords[index]->root;
		treeNode* before = current;

		//		msg(MSG_FATAL,"Searching predecessor of %s/%d",IPToString(ntohl(sip)).c_str(),sbits);
		while (current != NULL)
		{
			before = current;
			if (current->data.subnetBits == 32) return current;	

			uint32_t a = distance(match,current->left); 

			//check if our subnet is included in one of the child subnets 

			//		msg(MSG_FATAL,"Checking left node %s/%d",IPToString(current->left->data.subnetIP).c_str(),current->left->data.subnetBits);

			if (current->left->data.subnetBits <= 32- (uint32_t) (round(log(a)/log(2)+0.5)) && current->left->data.subnetBits <= sbits) 
			{
				//	msg(MSG_FATAL,"Subnet is included in left");
				current = current->left;
				continue;

			}


			uint32_t b = distance(match,current->right); 

			//		msg(MSG_FATAL,"Checking right node %s/%d",IPToString(current->right->data.subnetIP).c_str(),current->right->data.subnetBits);

			if (current->right->data.subnetBits <= 32- (uint32_t) (round(log(b)/log(2)+0.5)) && current->right->data.subnetBits <= sbits) 
			{

				//	msg(MSG_FATAL,"Subnet is included in right");
				current = current->right;
				continue;
			}

			//our subnet is not included in one of the child subnets, so there are 3 possible matches, left,right and current
			//		msg(MSG_FATAL,"its not included in any node");

			//now check if one of the child subnets is included in ours

			bool right = false;
			bool left = false;
			if ( sbits <= 32- (uint32_t) (round(log(a)/log(2)+0.5)) && sbits <= current->left->data.subnetBits )
			{
				left = true;
				//			msg(MSG_FATAL,"left subnet is included");
			}

			if ( sbits <= 32- (uint32_t) (round(log(b)/log(2)+0.5)) && sbits <= current->right->data.subnetBits )
			{
				//			msg(MSG_FATAL,"right subnet is included");
				right = true;
			}

			if (left && right) return current;
			if (left) return current->left;
			if (right) return current->right;

			return before;

		}

		return before;
	}

}
void AutoFocus::evaluate()
{

	uint32_t index = (m_treeCount-1) % numTrees;


	//msg(MSG_FATAL,"evaluating index %d",index);
	treeRecord* currentTree = m_treeRecords[index];
	treeRecord* last_tree = m_treeRecords[(index+numTrees-1) %numTrees];

	list<treeNode*>::iterator iter = currentTree->specNodes.begin();

	msg(MSG_FATAL,"---------------------");
	switch (lg_type) {
		case AutoFocus::lg_payload: 
			msg(MSG_FATAL,"Sort Critiera: payload");
			break;
		case AutoFocus::lg_fanouts:
			msg(MSG_FATAL,"Sort Criteria: fanouts");
			break;
		default: break; 
	}


	while (iter != currentTree->specNodes.end()) 
	{

		double percentage = 0;
		treeNode* last_value = getComparismValue(*iter,(index+numTrees-1) % numTrees);

		switch (lg_type) {
			case AutoFocus::lg_payload: 

				percentage = (double) ((*iter)->data.payload*100) / (double) currentTree->totalTraffic;	

				if (last_value) 
				{
					double last_percentage = (double) (last_value->data.payload*100) / (double) last_tree->totalTraffic;	
					double change = (double) ((*iter)->data.payload * 100) / (double) last_value->data.payload - 100.0;

					msg(MSG_FATAL,"SUBNET: %s/%d\t\t TRAFFIC: %03llu MB (%01.2f%%)\t PREVIOUSLY: %03llu MB (%01.2f%%)\t CHANGE: (%01.2f%%)\t",
							IPToString((*iter)->data.subnetIP).c_str(),(*iter)->data.subnetBits,
							(*iter)->data.payload/1024/1024,percentage,
							last_value->data.payload /1024/1024,last_percentage,
							change);
				}
				else 
				{
					msg(MSG_FATAL,"SUBNET: %s/%d\t\t TRAFFIC: %03llu MB (%01.2f%%)\t",
							IPToString((*iter)->data.subnetIP).c_str(),(*iter)->data.subnetBits,
							(*iter)->data.payload/1024/1024,percentage);
				}
				break;


			case AutoFocus::lg_fanouts:

				percentage = (double) ((*iter)->data.fanouts*100) / (double) currentTree->totalFanouts;	


				if (last_value) 
				{
					double last_percentage = (double) (last_value->data.fanouts*100) / (double) last_tree->totalFanouts;	
					double change = (double) ((*iter)->data.fanouts * 100) / (double) last_value->data.fanouts - 100.0;

					msg(MSG_FATAL,"SUBNET: %s/%d\t FANOUTS: %03llu (%01.2f%%)\t PREVIOUSLY: %03llu (%01.2f%%)\t CHANGE: %01.2f%%\t",
							IPToString((*iter)->data.subnetIP).c_str(),(*iter)->data.subnetBits,
							(*iter)->data.fanouts,percentage,
							last_value->data.fanouts,last_percentage,
							change);
				}
				else 
				{
					msg(MSG_FATAL,"SUBNET: %s/%d\t	FANOUTS: %03llu (%01.2f%%)\t",
							IPToString((*iter)->data.subnetIP).c_str(),(*iter)->data.subnetBits,
							(*iter)->data.fanouts,percentage);
				}
				break;

			default: break; 
		}


		iter++;
	}

	switch (lg_type) {
		case lg_payload:
			if (last_tree) 
			{
				msg(MSG_FATAL,"Total: \t%03llu\t %03llu\t (%01.2f%%)\t",currentTree->totalTraffic,last_tree->totalTraffic,(double) currentTree->totalTraffic * 100.0 / (double) last_tree->totalTraffic - 100.0);
			}
			else (MSG_FATAL,"Total \t%03llu\t",currentTree->totalTraffic);

			break;
		case lg_fanouts:
			if (last_tree)
			{
				msg(MSG_FATAL,"Total: \t%03llu\t %03llu\t (%01.2f%%)\t",currentTree->totalFanouts,last_tree->totalFanouts,(double) currentTree->totalFanouts * 100.0 / (double) last_tree->totalFanouts - 100.0);
			}
			else (MSG_FATAL,"Total \t%03llu\t",currentTree->totalFanouts);
			break;
		default:break;
	}

	msg(MSG_FATAL,"---------------------");
	/*
	   treeNode* left =  currentTree->root->left;
	   msg(MSG_FATAL,"%s/%d/t/t %03llu\t %03llu",IPToString(left->data.subnetIP).c_str(),left->data.subnetBits,left->data.fanouts,left->delta);
	   left = currentTree->root->right;
	   msg(MSG_FATAL,"%s/%d/t/t %03llu\t %03llu",IPToString(left->data.subnetIP).c_str(),left->data.subnetBits,left->data.fanouts,left->delta);
	   */
}


void AutoFocus::cleanUp()
{


	for (uint32_t i=0; i<hashSize; i++) {
		if (listIPRecords[i].size()==0) continue;

		list<IPRecord*>::iterator iter = listIPRecords[i].begin();
		while (iter != listIPRecords[i].end()) {
			delete *iter;
			iter++;
		}
		listIPRecords[i].clear();
	}
	totalData = 0;	
	statEntriesAdded = 0;	

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

	treeRecord* curTreeRecord = new treeRecord;

	switch (lg_type) {
		case AutoFocus::lg_payload:
			curTreeRecord->totalTraffic = totalData;
			break;
		case AutoFocus::lg_fanouts:
			curTreeRecord->totalFanouts = totalData;
		default: break;
	}

	//msg(MSG_FATAL,"Values %lu %u",totalData,numMaxResults);

	uint64_t threshold = (uint64_t) (totalData / numMaxResults);

	msg(MSG_FATAL,"THRESHOLD %lu",threshold);
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
			entry->data.fanouts = (*iter)->fanouts;
			entry->left = NULL;
			entry->right = NULL;
			switch (lg_type) {
				case AutoFocus::lg_payload: {
								    entry->delta = (*iter)->payload;
								    break;
							    }
				case AutoFocus::lg_fanouts: {
								    entry->delta = (*iter)->fanouts;
								    break;
							    }
				default: break;
			}

			//		msg(MSG_FATAL,"%lu",entry->delta);
			tree.push_back(entry);
			iter++;	
		}
	}

	tree.sort(AutoFocus::comp_entries);

	//* BUILD TREE *//

	list<treeNode*>::iterator iter = tree.begin();
	int count = 0;

	while (iter != tree.end())
	{


		if ((*iter)->delta > threshold) 
		{
			(*iter)->ddata = (*iter)->delta;
			(*iter)->delta = 0;
			curTreeRecord->specNodes.push_back(*iter);
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
			uint64_t f1 = (*iter)->data.fanouts;

			iter = tree.erase(iter);
			newnode->right = *iter;

			uint32_t ip2 = ntohl(((*iter)->data).subnetIP);
			uint64_t d2 = (*iter)->delta;
			uint64_t p2 = (*iter)->data.payload; 
			uint64_t f2 = (*iter)->data.fanouts;


			newnode->delta = d1+d2;


			newnode->data.payload = p1+p2;
			newnode->data.fanouts = f1+f2;

			iter = tree.erase(iter);

			int subbits = (uint32_t) (round(log(a)/log(2)+0.5));

			newnode->data.subnetIP = htonl((ip1>>subbits)<<subbits);
			newnode->data.subnetBits = 32 - subbits;


			if (newnode->delta > threshold) 
			{
				newnode->ddata = newnode->delta;
				newnode->delta = 0;
				curTreeRecord->specNodes.push_back(newnode);
			}

			iter--;
			if (*iter != tree.front()) iter--;
		}
		else
			iter++;



	}

	curTreeRecord->root = tree.front();


	if (m_treeRecords[m_treeCount % numTrees] != NULL)

	{
		deleteRecord(m_treeCount % numTrees);	

	}


	m_treeRecords[m_treeCount % numTrees] = curTreeRecord;
	m_treeCount++;
	evaluate();
	cleanUp();

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
	oss << "<totalData>" << totalData << "</totalData>" << endl;
	oss << "<nexttreein>" << timeTreeInterval - (time(0) - lastTreeBuilt)   << "</nexttreein>" << endl;
	oss << "</AutoFocus>" << endl;
	return oss.str();
}


