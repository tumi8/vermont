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

#if !defined(FLOWSIGMATCHER_H)
#define FLOWSIGMATCHER_H

#include "modules/idmef/IDMEFExporter.h"
#include "modules/ipfix/IpfixRecordDestination.h"
#include "modules/ipfix/Connection.h"
#include "core/Source.h"

#include <boost/regex.hpp>
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>
#include <list>
#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <vector>

using namespace std;

typedef struct {
        uint32_t ip;
        uint16_t mask;
} ipEntry;

typedef struct {
        uint32_t protocol;
        uint16_t srcPort;
        uint16_t srcPortEnd;
        uint16_t dstPort;
        uint16_t dstPortEnd;
        list<ipEntry*> src;
        list<ipEntry*> dst;
        uint32_t uid;
        string type;
        string source;
        string msg;
} IdsRule;

class GenNode {
	public:
	enum GenType {
		proto,
		srcIP,
		dstIP,
		srcPort,
		dstPort,
		rule
	};
	static GenType order[6];
	static GenNode* newGenNode(int depth);
	static void parse_order(string order);
	virtual void findRule(Connection* conn, list<IdsRule*>& rules)=0;
	virtual void insertRule(IdsRule* rule,int depth)=0;
	virtual ~GenNode() {};
};

class ProtoNode : public GenNode {
	GenNode* any;
	GenNode* udp;
	GenNode* tcp;
	GenNode* icmp;
	public:
	virtual void findRule(Connection* conn, list<IdsRule*>& rules);
	virtual void insertRule(IdsRule* rule,int depth);
        ProtoNode();
	~ProtoNode();
};

class SrcIpNode : public GenNode {
	GenNode* any;
	map<uint32_t,GenNode*> ipmaps[4];
	public:
	virtual void findRule(Connection* conn, list<IdsRule*>& rules);
	virtual void insertRule(IdsRule* rule,int depth);
        SrcIpNode();
	~SrcIpNode();
};

class DstIpNode : public GenNode {
	GenNode* any;
	map<uint32_t,GenNode*> ipmaps[4];
	public:
	virtual void findRule(Connection* conn, list<IdsRule*>& rules);
	virtual void insertRule(IdsRule* rule,int depth);
        DstIpNode();
	~DstIpNode();
};

class SrcPortNode : public GenNode {
	GenNode* any;
	map<uint16_t,GenNode*> portmap;
	public:
	virtual void findRule(Connection* conn, list<IdsRule*>& rules);
	virtual void insertRule(IdsRule* rule,int depth);
        SrcPortNode();
	~SrcPortNode();
};

class DstPortNode : public GenNode {
	GenNode* any;
	map<uint16_t,GenNode*> portmap;
	public:
	virtual void findRule(Connection* conn, list<IdsRule*>& rules);
	virtual void insertRule(IdsRule* rule,int depth);
        DstPortNode();
        ~DstPortNode();
};

class RuleNode : public GenNode {
	list<IdsRule*> rulesList;
	public:
	virtual void findRule(Connection* conn, list<IdsRule*>& rules);
	virtual void insertRule(IdsRule* rule,int depth);
	~RuleNode();
};

class FlowSigMatcher
	: public Module,
	  public IpfixRecordDestination,
	  public Source<IDMEFMessage*>
{
	public:
		FlowSigMatcher(string homenet, string rulesfile, string rulesorder, string analyzerid, string idmeftemplate);
		virtual ~FlowSigMatcher();

		virtual void onDataRecord(IpfixDataRecord* record);

	private:
                ifstream infile;
                string homenet;
                string rulesfile;
		string analyzerId;	/**< analyzer id for IDMEF messages */
		string idmefTemplate;	/**< template file for IDMEF messages */


		// idmef parameters
		const static char* PAR_SOURCE_PORT; // = "SOURCE_PORT";
		const static char* PAR_TARGET_PORT; // = "TARGET_PORT";
		const static char* PAR_SOURCE; // = "SOURCE";
		const static char* PAR_UID; // = "UID";
		const static char* PAR_TYPE; // = "TYPE";
		const static char* PAR_MSG; // = "MSM";
	
                list<IdsRule*> parsedRules;
                GenNode* treeRoot;

		// manages instances of IDMEFMessages
		static InstanceManager<IDMEFMessage> idmefManager;
                int parse_line(string text);
                int parse_port(string text, IdsRule& rule, uint32_t dst);
                void split_ip(string text, list<ipEntry*>& list);
                int parse_ip(string text, IdsRule& rule, uint32_t dst);
		/*void addConnection(Connection* conn);
		virtual string getStatisticsXML(double interval);
		void cleanupEntries();*/
};

#endif
