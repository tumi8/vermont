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

#include "modules/idmef/IDMEFExporter.h"
#include "modules/ipfix/IpfixRecordDestination.h"
#include "modules/ipfix/Connection.h"
#include "core/Source.h"
#include "autofocus_iprecord.h"
#include "autofocus_attribute.h"
#include "autofocus_report.h"
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


		AutoFocus(uint32_t hashbits
				, uint32_t ttreeint,uint32_t nummaxr, uint32_t numtrees, uint32_t subbits, string analyzerid, string reportfile);
		virtual ~AutoFocus();
		virtual void onDataRecord(IpfixDataRecord* record);

	private:



		uint32_t hashSize;
		uint32_t hashBits;	/**< amount of bits used for hashtable */
		uint32_t timeTreeInterval; // time in seconds when tree is rebuilt
		uint32_t lastTreeBuilt;
		uint32_t numMaxResults;
		uint32_t numTrees;
		uint32_t m_minSubbits;

		string analyzerId;	/**< analyzer id for IDMEF messages */
		string reportfile;	/**< template file for IDMEF messages */
		uint32_t statEntriesAdded;

		vector<treeRecord*> m_treeRecords;
		uint32_t m_treeCount;

		list<IPRecord*>* listIPRecords;
	

		uint32_t distance(treeNode*,treeNode*);
		static bool comp_entries(treeNode*,treeNode*);
		static bool metasort(treeNode*,treeNode*);



		void buildTree();
		void cleanUp();
		void evaluate();
		void metalist();
		void deleteRecord(int);
		void deleteTree(treeNode*);
		void initiateRecord(int);
		IPRecord* createEntry(Connection* conn);
		IPRecord* getEntry(Connection* conn);

		void addConnection(Connection* conn);
		void check_node(treeRecord*,treeNode*);
		void aggregate_newnode(treeNode*);

		// manages instances of IDMEFMessages
		static InstanceManager<IDMEFMessage> idmefManager;

		virtual string getStatistics();
		virtual std::string getStatisticsXML(double);
};


#endif
