
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
#include "autofocus_iprecord.h"

InstanceManager<IDMEFMessage> AutoFocus::idmefManager("IDMEFMessage");

/**
 * attention: parameter idmefexporter must be free'd by the creating instance, TRWPortWormDetector
 * does not dare to delete it, in case it's used
 */
	AutoFocus::AutoFocus(uint32_t hashbits, uint32_t ttreeint, uint32_t nummaxr, uint32_t numtrees, string analyzerid, string idmeftemplate)
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

	m_treeCount = 0;
	m_treeRecords.clear();



	listIPRecords = new list<IPRecord*>[hashSize];
	initiateRecord(m_treeCount % numTrees);
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

	std::map<report_enum,af_attribute*>::iterator iter = te->m_attributes.begin();

	while (iter != te->m_attributes.end())
	{
		(iter->second)->aggregate(te,conn);
		iter++;
	}

}

/**
 * returns entry in hashtable for the given connection
 * if it was not found, a new entry is created and returned
 */
IPRecord* AutoFocus::getEntry(Connection* conn)
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

	std::map<report_enum,af_attribute*>::iterator iter = root->data.m_attributes.begin();

	while (iter != root->data.m_attributes.end())	
	{
		delete (iter->second);	
		iter++;
	}
	delete root;
	return;
}

IPRecord* AutoFocus::createEntry(Connection* conn)

{
	IPRecord* te = new IPRecord();
	te->subnetIP = conn->srcIP;
	te->subnetBits = 32;

	std::list<report*>::iterator iter  = m_treeRecords[m_treeCount % numTrees]->reports.begin();

	while (iter != m_treeRecords[m_treeCount % numTrees]->reports.end())
	{
		te->m_attributes[(*iter)->getID()] = (*iter)->createaf_attribute();
		iter++;
	}
	statEntriesAdded++;
	return te;
}


void AutoFocus::deleteRecord(int index)
{
	msg(MSG_FATAL,"Deleting Index %d",index);
	deleteTree(m_treeRecords[index]->root);	

	std::list<report*>::iterator iter = m_treeRecords[index]->reports.begin();
	while (iter != m_treeRecords[index]->reports.end())
	{
		delete *iter;
		iter++;
	}

	delete m_treeRecords[index];
}



void AutoFocus::evaluate()
{

	uint32_t index = (m_treeCount-1) % numTrees;


	//msg(MSG_FATAL,"evaluating index %d",index);
	treeRecord* currentTree = m_treeRecords[index];
	treeRecord* last_tree = m_treeRecords[(index+numTrees-1) %numTrees];


	std::list<report*>::iterator iter = currentTree->reports.begin();

	while (iter != currentTree->reports.end())
	{
		(*iter)->post(currentTree,last_tree,index);
		iter++;
	}

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
	statEntriesAdded = 0;	

}

uint32_t AutoFocus::distance(treeNode* a,treeNode* b) 
{
	return ntohl(a->data.subnetIP) ^ ntohl(b->data.subnetIP);
}

void AutoFocus::initiateRecord(int index) 
{
	if (m_treeRecords[index % numTrees ] != NULL)
		deleteRecord(index % numTrees);

	m_treeRecords[index % numTrees] = new treeRecord;
	m_treeRecords[index % numTrees]->reports.push_back(new rep_payload_tcp());
	m_treeRecords[index % numTrees]->reports.push_back(new rep_payload_udp());
	m_treeRecords[index % numTrees]->reports.push_back(new rep_fanouts());


}
void AutoFocus::buildTree () 
{
	msg(MSG_FATAL,"STARTING TREE BUILDING");
	treeNode* root;
	list<treeNode*> tree;

	treeRecord* curTreeRecord = m_treeRecords[m_treeCount % numTrees];


	for (uint32_t i=0; i<hashSize; i++) {
		if (listIPRecords[i].size()==0) continue;

		list<IPRecord*>::iterator iter = listIPRecords[i].begin();

		while (iter != listIPRecords[i].end()) 
		{
			treeNode* entry = new treeNode;
			entry->data.subnetIP = (*iter)->subnetIP;
			entry->data.subnetBits = (*iter)->subnetBits;
			entry->left = NULL;
			entry->right = NULL;
			entry->data.m_attributes = (*iter)->m_attributes;


			std::map<report_enum,af_attribute*>::iterator iter2 = (*iter)->m_attributes.begin();

			while (iter2 != (*iter)->m_attributes.end())
			{
				(iter2->second)->delta = (iter2->second)->numCount;
				iter2++;
			}
			/*
			   std::map<report_enum,af_attribute*>::iterator iter2 = (*iter)->m_attributes.begin();

			   while (iter2 != (*iter)->m_attributes.end())
			   {

			   entry->data.m_attributes[iter2->first] = (iter2->second)->getCopy();
			   iter2++;
			   }
			   */
			tree.push_back(entry);
			iter++;	
		}
	}

	msg(MSG_FATAL,"converted iprecords to treenodes");

	tree.sort(AutoFocus::comp_entries);

	msg(MSG_FATAL,"everything is sorted %d",tree.size());
	//* BUILD TREE *//

	list<treeNode*>::iterator iter = tree.begin();
	int count = 0;

	while (iter != tree.end())
	{


		check_node(curTreeRecord,*iter);

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

			iter = tree.erase(iter);

			newnode->right = *iter;

			check_node(curTreeRecord,*iter);

			uint32_t ip2 = ntohl(((*iter)->data).subnetIP);

			aggregate_newnode(newnode);


			iter = tree.erase(iter);

			int subbits = (uint32_t) (round(log(a)/log(2)+0.5));

			newnode->data.subnetIP = htonl((ip1>>subbits)<<subbits);
			newnode->data.subnetBits = 32 - subbits;

			check_node(curTreeRecord,newnode);

			iter--;
			if (*iter != tree.front()) iter--;
		}
		else
			iter++;



	}

	curTreeRecord->root = tree.front();

	m_treeCount++;

	initiateRecord(m_treeCount % numTrees);

	evaluate();
	cleanUp();

}

void AutoFocus::aggregate_newnode(treeNode* newnode)
{

	std::list<report*>::iterator iter  = m_treeRecords[m_treeCount % numTrees]->reports.begin();

	while (iter != m_treeRecords[m_treeCount % numTrees]->reports.end())
	{
		newnode->data.m_attributes[(*iter)->getID()] = (*iter)->createaf_attribute();
		newnode->data.m_attributes[(*iter)->getID()]->collect(newnode->left->data.m_attributes[(*iter)->getID()],newnode->right->data.m_attributes[(*iter)->getID()]);
		iter++;
	}
}

void AutoFocus::check_node(treeRecord* curTreeRecord,treeNode* newnode)
{

	std::list<report*>::iterator iter = curTreeRecord->reports.begin();

	while (iter != curTreeRecord->reports.end())
	{
		(*iter)->checkNode(newnode,numMaxResults);
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

treeNode* AutoFocus::getComparismValue(treeNode* match,uint32_t index)
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


