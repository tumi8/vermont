/*
  VERMONT 
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

#ifndef AUTOFOCUS_H
#define AUTOFOCUS_H

#include "idmef/IDMEFExporter.h"
#include "IpfixRecordDestination.h"
#include "Connection.h"
#include "reconf/Source.h"

#include <list>
#include <string>
#include <vector>

using namespace std;

class AutoFocus
: public Module, 
	public IpfixRecordDestination, 
	public Source<IDMEFMessage*>
{
	public:
		
		enum logtype { lg_payload, lg_fanouts };
		AutoFocus(uint32_t hashbits
				, uint32_t ttreeint,uint32_t nummaxr, uint32_t numtrees, string analyzerid, string idmeftemplate);
		virtual ~AutoFocus();

		virtual void onDataDataRecord(IpfixDataDataRecord* record);
	private:

		typedef struct IPRecord {

			uint32_t subnetIP;
			uint32_t subnetBits;
			uint64_t payload;
			uint64_t fanouts;
		}IPRecord;

		typedef	struct treeNode {
			IPRecord data;
			uint64_t delta;
			uint64_t ddata;
			treeNode* left;
			treeNode* right;
		}treeNode;

		typedef struct treeRecord {
			treeNode* root;
			list<treeNode*> specNodes;
			uint64_t totalTraffic;
			uint64_t totalFanouts;
			}treeRecord;


		uint64_t totalData;

		uint32_t hashSize;
		uint32_t hashBits;	/**< amount of bits used for hashtable */
		uint32_t timeTreeInterval; // time in seconds when tree is rebuilt
		uint32_t lastTreeBuilt;
		uint32_t numMaxResults;
		uint32_t numTrees;
		logtype lg_type;

		string analyzerId;	/**< analyzer id for IDMEF messages */
		string idmefTemplate;	/**< template file for IDMEF messages */
		uint32_t statEntriesAdded;

		vector<treeRecord*> m_treeRecords;
		uint32_t m_treeCount;

		list<IPRecord*>* listIPRecords;


		uint32_t distance(treeNode*,treeNode*);
		static bool comp_entries(treeNode*,treeNode*);

		void buildTree();
		void cleanUp();
		void evaluate();
		void deleteRecord(int);
		void deleteTree(treeNode*);
		treeNode* getComparismValue(treeNode*,uint32_t);
		IPRecord* createEntry(Connection* conn);
		IPRecord* getEntry(Connection* conn);

		void addConnection(Connection* conn);


		// manages instances of IDMEFMessages
		static InstanceManager<IDMEFMessage> idmefManager;

		virtual string getStatistics();
		virtual std::string getStatisticsXML();
};

#endif
