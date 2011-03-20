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

#include "FlowSigMatcher.h"
#include "common/crc.hpp"
#include "common/Misc.h"

#include <arpa/inet.h>
#include <math.h>
#include <iostream>

GenNode::GenType GenNode::order[6]={ proto, srcIP, dstIP, srcPort, dstPort, rule };
const char* FlowSigMatcher::PAR_SOURCE_PORT = "SOURCE_PORT";
const char* FlowSigMatcher::PAR_TARGET_PORT = "TARGET_PORT";
const char* FlowSigMatcher::PAR_SOURCE = "SOURCE";
const char* FlowSigMatcher::PAR_UID = "UID";
const char* FlowSigMatcher::PAR_TYPE = "TYPE";
const char* FlowSigMatcher::PAR_MSG = "MSG";


InstanceManager<IDMEFMessage> FlowSigMatcher::idmefManager("FlowSigMatcherIDMEFMessage", 0);

/**
 * attention: parameter idmefexporter must be free'd by the creating instance, FlowSigMatcher
 * does not dare to delete it, in case it's used
 */
FlowSigMatcher::FlowSigMatcher(string homenet, string rulesfile, string rulesorder, string analyzerid, string idmeftemplate, string flagstimeout)
	: homenet(homenet),
	  rulesfile(rulesfile),
	  analyzerId(analyzerid),
	  idmefTemplate(idmeftemplate),
	  flagsTimeout(10000)
{
	const boost::regex exp_valid_homenet("(\\[.*\\]|(!?\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})(?:/(\\d+))*)");
	boost::cmatch what;
	if(!boost::regex_match(homenet.c_str(),what,exp_valid_homenet)) THROWEXCEPTION("HOMENET is invalid: %s",homenet.c_str());
	GenNode::parse_order(rulesorder);
	if(flagstimeout.compare("")!=0) flagsTimeout=strtoull(flagstimeout.c_str(),NULL,10);
	string buffer;
	infile.open(rulesfile.c_str(),ifstream::in);
	if(!infile.is_open()) {
		THROWEXCEPTION("Couldn't open rulesfile! %s", rulesfile.c_str());
		return;
	}
	while(getline(infile,buffer)) {
		if(parse_line(buffer)==false) {
		msg(MSG_DIALOG,"Couldn't parse this rule: %s",buffer.c_str());
		}
	}
	msg(MSG_DIALOG, "added %i rules",parsedRules.size());
	infile.close();
	list<IdsRule*>::iterator it;
	treeRoot=GenNode::newNode(0);
	for(it=parsedRules.begin();it!=parsedRules.end();it++) {
		treeRoot->insertRule(*it,0);
		if((*it)->mode>0&&(*it)->mode!=3) treeRoot->insertRevRule(*it,0);
	}
}

FlowSigMatcher::~FlowSigMatcher()
{
	delete treeRoot;
	list<IdsRule*>::iterator it;
        list<FlagsRule*>::iterator flagIt;
	for(it=parsedRules.begin();it!=parsedRules.end();it++) {
		list<IpEntry*>::iterator ipIt;
		list<PortEntry*>::iterator portIt;
		for(ipIt=(*it)->src.begin();ipIt!=(*it)->src.end();ipIt++) delete (*ipIt);
		for(ipIt=(*it)->dst.begin();ipIt!=(*it)->dst.end();ipIt++) delete (*ipIt);
		for(portIt=(*it)->sPort.begin();portIt!=(*it)->sPort.end();portIt++) delete (*portIt);
		for(portIt=(*it)->dPort.begin();portIt!=(*it)->dPort.end();portIt++) delete (*portIt);
		delete (*it); 
	}
	for(flagIt=flagRules.begin();flagIt!=flagRules.end();flagIt++) delete (*flagIt); 
}

void FlowSigMatcher::onDataRecord(IpfixDataRecord* record)
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
/*	if (conn.srcTcpControlBits&Connection::SYN) {
		addConnection(&conn);
	}*/
        set<IdsRule*> matchingRules;
        treeRoot->findRule(&conn,matchingRules);
        set<IdsRule*>::iterator it;
        for(it=matchingRules.begin();it!=matchingRules.end();it++) {
		if(((*it)->mode>=2)&&(conn.srcPackets==0||conn.dstPackets==0)) continue; //check if flows are bidir
		if(((*it)->mode>=3)&&(conn.protocol==6)) { //check for tcp 3-way handshake
			if((conn.srcTcpControlBits&(Connection::SYN|Connection::ACK))!=(Connection::SYN|Connection::ACK)) continue;
			if((conn.dstTcpControlBits&(Connection::SYN|Connection::ACK))!=(Connection::SYN|Connection::ACK)) continue;
			if(ntohll(conn.srcOctets)/ntohll(conn.srcPackets)<44) continue;
			if(ntohll(conn.dstOctets)/ntohll(conn.dstPackets)<44) continue;
		}
		if((*it)->flag==0) {
			sendMessage(conn, (*it)->source, (*it)->type, (*it)->uid, (*it)->msg); 
		}
		else {
			map< uint32_t, map<uint32_t,uint64_t> >::iterator flagIpIt;
			map< uint32_t, uint64_t >::iterator flagIt;
			uint64_t latestTime;
			if (conn.dstTimeEnd>conn.srcTimeEnd) latestTime=conn.dstTimeEnd;
			else latestTime=conn.srcTimeEnd;
			//if (activeFlags[0][(*it)->flag]<latestTime) activeFlags[0][(*it)->flag]=latestTime;
			if (activeFlags[htonl(conn.srcIP)][(*it)->flag]<latestTime) activeFlags[htonl(conn.srcIP)][(*it)->flag]=latestTime;
			if (activeFlags[htonl(conn.dstIP)][(*it)->flag]<latestTime) activeFlags[htonl(conn.dstIP)][(*it)->flag]=latestTime;
			for(flagIpIt=activeFlags.begin();flagIpIt!=activeFlags.end();flagIpIt++) {
				for(flagIt=flagIpIt->second.begin();flagIt!=flagIpIt->second.end();flagIt++) { //delete outdated flags
					if((flagIt->second < conn.srcTimeStart)&&(conn.srcTimeStart - flagIt->second > flagsTimeout)) flagIpIt->second.erase(flagIt);
				}
				if(flagIpIt->second.empty()) {//delete empty states from map
					activeFlags.erase(flagIpIt);
				}
			}
			list<FlagsRule*>::iterator flagRulesIt;
			for(flagRulesIt=flagRules.begin();flagRulesIt!=flagRules.end();flagRulesIt++) {
				set<uint32_t>::iterator it;
				if((flagIpIt=activeFlags.find(htonl(conn.srcIP)))!=activeFlags.end()) {
					for(it=(*flagRulesIt)->flags.begin();it!=(*flagRulesIt)->flags.end();it++) {
						if(flagIpIt->second.find(*it) == flagIpIt->second.end()) break;
					}
					if(it==(*flagRulesIt)->flags.end()) {
						sendMessage(conn, (*flagRulesIt)->source, (*flagRulesIt)->type, (*flagRulesIt)->uid, (*flagRulesIt)->msg); 
					}
				}
				if((flagIpIt=activeFlags.find(htonl(conn.dstIP)))!=activeFlags.end()) {
					for(it=(*flagRulesIt)->flags.begin();it!=(*flagRulesIt)->flags.end();it++) {
						if(flagIpIt->second.find(*it) == flagIpIt->second.end()) break;
					}
					if(it==(*flagRulesIt)->flags.end()) {
						sendMessage(conn, (*flagRulesIt)->source, (*flagRulesIt)->type, (*flagRulesIt)->uid, (*flagRulesIt)->msg); 
					}
				}
			}
		}
        }
	record->removeReference();
}

void FlowSigMatcher::sendMessage(Connection& conn,uint8_t source, uint8_t type, uint32_t uid, string message) 
{
	msg(MSG_DIALOG, "intruder detected:");
	msg(MSG_DIALOG, "srcIP: %s, dstIP: %s, srcPort: %i dstPort: %i", IPToString(conn.srcIP).c_str(),
			IPToString(conn.dstIP).c_str(), ntohs(conn.srcPort), ntohs(conn.dstPort));
	IDMEFMessage* msg = idmefManager.getNewInstance();
	msg->init(idmefTemplate, analyzerId);
	stringstream ssrcPort;
	ssrcPort << ntohs(conn.srcPort);
	stringstream sdstPort;
	sdstPort << ntohs(conn.dstPort);
	msg->setVariable(IDMEFMessage::PAR_SOURCE_ADDRESS, IPToString(conn.srcIP));
	msg->setVariable(IDMEFMessage::PAR_TARGET_ADDRESS, IPToString(conn.dstIP));
	msg->setVariable(PAR_SOURCE_PORT, ssrcPort.str());
	msg->setVariable(PAR_TARGET_PORT, sdstPort.str());
	msg->setVariable(PAR_SOURCE, idsRuleSource.at(source));
	msg->setVariable(PAR_UID, uid);
	msg->setVariable(PAR_TYPE, idsRuleType.at(type));
	msg->setVariable(PAR_MSG, message);
	msg->applyVariables();
	send(msg);
}

void GenNode::parse_order(string order) 
{
	if(order.compare("")==0) return;
	boost::char_separator<char> sep(", ");
	boost::tokenizer<boost::char_separator<char> > tokens(order, sep);
	int i=0;
	BOOST_FOREACH(string t, tokens) {
	if(t.compare("srcIP")==0) GenNode::order[i]=srcIP;
	else if(t.compare("dstIP")==0) GenNode::order[i]=dstIP;
	else if(t.compare("srcPort")==0) GenNode::order[i]=srcPort;
	else if(t.compare("dstPort")==0) GenNode::order[i]=dstPort;
	else if(t.compare("proto")==0) GenNode::order[i]=proto;
	else THROWEXCEPTION("FlowSigMatcher: rulesorder - rulesorder may only contain the following keywords and each has to be mentioned exactly once: srcIP, dstIP, srcPort, dstPort, proto");
	i++;
	}
	GenNode::order[i]=rule;
	int tmp[5]={0};
	for(i=0;i<5;i++) tmp[GenNode::order[i]]+=1;
	for(i=0;i<5;i++) {
		if(tmp[i]>1) THROWEXCEPTION("FlowSigMatcher: rulesorder - same GenNode Types selected more than once");
		else if(tmp[i]==0) THROWEXCEPTION("FlowSigMatcher: rulesorder - not all GenNode Types selected");
	}
}


ProtoNode::ProtoNode() : any(NULL), udp(NULL), tcp(NULL), icmp(NULL) {}

void ProtoNode::findRule(Connection* conn,set<IdsRule*>& rules) 
{
	if(any!=NULL) any->findRule(conn,rules);
	if(conn->protocol==6) {
		if(tcp!=NULL)	tcp->findRule(conn,rules);
	}
	else if(conn->protocol==17) {
		if(udp!=NULL) udp->findRule(conn,rules);
	}
	else if(conn->protocol==1) {
		if(icmp!=NULL) icmp->findRule(conn,rules);
	}
}

void ProtoNode::invalidateRule(Connection* conn,set<IdsRule*>& rules) 
{
	if(any!=NULL) any->invalidateRule(conn,rules);
	if(conn->protocol==6) {
		if(tcp!=NULL)	tcp->invalidateRule(conn,rules);
	}
	else if(conn->protocol==17) {
		if(udp!=NULL) udp->invalidateRule(conn,rules);
	}
	else if(conn->protocol==1) {
		if(icmp!=NULL) icmp->invalidateRule(conn,rules);
	}
}

void ProtoNode::insertRule(IdsRule* rule,int depth) 
{
	if(rule->protocol==6) { //TCP
		if(tcp==NULL) tcp=newNode(depth+1);
		tcp->insertRule(rule,depth+1);
	}
        else if(rule->protocol==17) { //UDP
		if(udp==NULL) udp=newNode(depth+1);
		udp->insertRule(rule,depth+1);
        }
        else if(rule->protocol==1) { //ICMP
		if(icmp==NULL) icmp=newNode(depth+1);
		icmp->insertRule(rule,depth+1);
	} else {
		if(any==NULL) any=newNode(depth+1);
		any->insertRule(rule,depth+1);
	}
}

void ProtoNode::insertRevRule(IdsRule* rule,int depth) 
{
	if(rule->protocol==6) { //TCP
		if(tcp==NULL) tcp=newNode(depth+1);
		tcp->insertRevRule(rule,depth+1);
	}
        else if(rule->protocol==17) { //UDP
		if(udp==NULL) udp=newNode(depth+1);
		udp->insertRevRule(rule,depth+1);
        }
        else if(rule->protocol==1) { //ICMP
		if(icmp==NULL) icmp=newNode(depth+1);
		icmp->insertRevRule(rule,depth+1);
	} else {
		if(any==NULL) any=newNode(depth+1);
		any->insertRevRule(rule,depth+1);
	}
}

ProtoNode::~ProtoNode() 
{
	if(tcp!=NULL) delete tcp;
	if(udp!=NULL) delete udp;
	if(icmp!=NULL) delete icmp;
}
SrcPortNode::SrcPortNode() :any(NULL) {}

void SrcPortNode::findRule(Connection* conn,set<IdsRule*>& rules) 
{
	if(any!=NULL) any->findRule(conn,rules);
	map<uint16_t,GenNode*>::iterator it, notit;
	it=portmap.find(ntohs(conn->srcPort));
	if(it!=portmap.end()) {
		it->second->findRule(conn,rules);
	}
	
	list<PortListEntry*>::iterator listit;
	for(listit=portlist.begin();listit!=portlist.end();listit++) {
		if(((*listit)->entry->port<=ntohs(conn->srcPort))&&((*listit)->entry->portEnd>=ntohs(conn->srcPort))) {
			(*listit)->node->findRule(conn,rules);
		}
	}
	for(listit=notportlist.begin();listit!=notportlist.end();listit++) {
		if(((*listit)->entry->port<=ntohs(conn->srcPort))&&((*listit)->entry->portEnd>=ntohs(conn->srcPort))) 
		(*listit)->node->invalidateRule(conn,rules);
	}
	it=notportmap.find(ntohs(conn->srcPort));
	if(it!=notportmap.end()) it->second->invalidateRule(conn,rules);
}

void SrcPortNode::invalidateRule(Connection* conn, set<IdsRule*>& rules)
{
	if(any!=NULL) any->invalidateRule(conn,rules);
	map<uint16_t,GenNode*>::iterator it, notit;
	it=portmap.find(ntohs(conn->srcPort));
	if(it!=portmap.end()) it->second->invalidateRule(conn,rules);
	list<PortListEntry*>::iterator listit;		
	for(listit=portlist.begin();listit!=portlist.end();listit++) {
		if(((*listit)->entry->port<=ntohs(conn->srcPort))&&((*listit)->entry->portEnd>=ntohs(conn->srcPort))) 
		(*listit)->node->invalidateRule(conn,rules);
	}
	it=notportmap.find(ntohs(conn->srcPort));
	if(it!=notportmap.end()) it->second->invalidateRule(conn,rules);
	for(listit=notportlist.begin();listit!=notportlist.end();listit++) {
		if(((*listit)->entry->port<=ntohs(conn->srcPort))&&((*listit)->entry->portEnd>=ntohs(conn->srcPort))) 
			(*listit)->node->invalidateRule(conn,rules);
	}
}

void SrcPortNode::insertRule(IdsRule* rule,int depth) 
{
	int onlyNeg=1;
        list<PortEntry*>::iterator listit;
        for(listit=rule->sPort.begin();listit!=rule->sPort.end();listit++) {
		if((*listit)->notFlag==0) {
			onlyNeg=0;
			break;
		}
	}
	if(onlyNeg) {
		if(any==NULL) any=newNode(depth+1);
		any->insertRule(rule,depth+1);
	}
        for(listit=rule->sPort.begin();listit!=rule->sPort.end();listit++) {
		if((*listit)->port==0) {
			if(any==NULL) any=newNode(depth+1);
			any->insertRule(rule,depth+1);
		}
		else if((*listit)->port==(*listit)->portEnd) {//insert single port into map
			map<uint16_t,GenNode*>::iterator it;
			if((*listit)->notFlag==0) {
				it=portmap.find((*listit)->port);
				if(it==portmap.end()) portmap[(*listit)->port]=newNode(depth+1);
				portmap[(*listit)->port]->insertRule(rule,depth+1);
			}
			else {
				it=notportmap.find((*listit)->port);
				if(it==notportmap.end()) notportmap[(*listit)->port]=newNode(depth+1);
				notportmap[(*listit)->port]->insertRule(rule,depth+1);
			}
		}
		else if(((*listit)->portEnd!=0)&&((*listit)->port<(*listit)->portEnd)) {//insert port range into list
			if((*listit)->notFlag==0) {
				list<PortListEntry*>::iterator it;		
				for(it=portlist.begin();it!=portlist.end();it++) {
					if(((*listit)->port==(*it)->entry->port)&&((*listit)->portEnd==(*it)->entry->portEnd)) {
						(*it)->node->insertRule(rule,depth+1);
						break;
					}
				}
				if(it==portlist.end()) {
					PortListEntry* entry=new PortListEntry;	
					entry->entry=*listit;
					entry->node=newNode(depth+1);
					entry->node->insertRule(rule,depth+1);
					portlist.push_back(entry);
				}
			}
			else {
				list<PortListEntry*>::iterator it;		
				for(it=notportlist.begin();it!=notportlist.end();it++) {
					if(((*listit)->port==(*it)->entry->port)&&((*listit)->portEnd==(*it)->entry->portEnd)) {
						(*it)->node->insertRule(rule,depth+1);
						break;
					}
				}
				if(it==notportlist.end()) {
					PortListEntry* entry=new PortListEntry;	
					entry->entry=*listit;
					entry->node=newNode(depth+1);
					entry->node->insertRule(rule,depth+1);
					notportlist.push_back(entry);
				}
			}
			
		}
	}
}

void DstPortNode::insertRevRule(IdsRule* rule,int depth) 
{
	int onlyNeg=1;
        list<PortEntry*>::iterator listit;
        for(listit=rule->sPort.begin();listit!=rule->sPort.end();listit++) {
		if((*listit)->notFlag==0) {
			onlyNeg=0;
			break;
		}
	}
	if(onlyNeg) {
		if(any==NULL) any=newNode(depth+1);
		any->insertRevRule(rule,depth+1);
	}
        for(listit=rule->sPort.begin();listit!=rule->sPort.end();listit++) {
		if((*listit)->port==0) {
			if(any==NULL) any=newNode(depth+1);
			any->insertRevRule(rule,depth+1);
		}
		else if((*listit)->port==(*listit)->portEnd) {//insert single port into map
			map<uint16_t,GenNode*>::iterator it;
			if((*listit)->notFlag==0) {
				it=portmap.find((*listit)->port);
				if(it==portmap.end()) portmap[(*listit)->port]=newNode(depth+1);
				portmap[(*listit)->port]->insertRevRule(rule,depth+1);
			}
			else {
				it=notportmap.find((*listit)->port);
				if(it==notportmap.end()) notportmap[(*listit)->port]=newNode(depth+1);
				notportmap[(*listit)->port]->insertRevRule(rule,depth+1);
			}
		}
		else if(((*listit)->portEnd!=0)&&((*listit)->port<(*listit)->portEnd)) {//insert port range into list
			if((*listit)->notFlag==0) {
				list<PortListEntry*>::iterator it;		
				for(it=portlist.begin();it!=portlist.end();it++) {
					if(((*listit)->port==(*it)->entry->port)&&((*listit)->portEnd==(*it)->entry->portEnd)) {
						(*it)->node->insertRevRule(rule,depth+1);
						break;
					}
				}
				if(it==portlist.end()) {
					PortListEntry* entry=new PortListEntry;	
					entry->entry=*listit;
					entry->node=newNode(depth+1);
					entry->node->insertRevRule(rule,depth+1);
					portlist.push_back(entry);
				}
			}
			else {
				list<PortListEntry*>::iterator it;		
				for(it=notportlist.begin();it!=notportlist.end();it++) {
					if(((*listit)->port==(*it)->entry->port)&&((*listit)->portEnd==(*it)->entry->portEnd)) {
						(*it)->node->insertRevRule(rule,depth+1);
						break;
					}
				}
				if(it==notportlist.end()) {
					PortListEntry* entry=new PortListEntry;	
					entry->entry=*listit;
					entry->node=newNode(depth+1);
					entry->node->insertRevRule(rule,depth+1);
					notportlist.push_back(entry);
				}
			}
			
		}
	}
}

SrcPortNode::~SrcPortNode() 
{
	if(any!=NULL) delete any;
	map<uint16_t,GenNode*>::iterator it;
	for(it=portmap.begin();it!=portmap.end();it++) {
		if(it->second!=NULL)	delete it->second;
	}
	for(it=notportmap.begin();it!=notportmap.end();it++) {
		if(it->second!=NULL)	delete it->second;
	}
	list<PortListEntry*>::iterator listit;
	for(listit=portlist.begin();listit!=portlist.end();listit++) {
		delete (*listit)->node;
		delete (*listit);
	}
	
}

DstPortNode::DstPortNode() : any(NULL) {}

void DstPortNode::findRule(Connection* conn,set<IdsRule*>& rules) 
{
	if(any!=NULL) any->findRule(conn,rules);
	map<uint16_t,GenNode*>::iterator it, notit;
	it=portmap.find(ntohs(conn->dstPort));
	if(it!=portmap.end()) {
		it->second->findRule(conn,rules);
	}
	
	list<PortListEntry*>::iterator listit;
	for(listit=portlist.begin();listit!=portlist.end();listit++) {
		if(((*listit)->entry->port<=ntohs(conn->dstPort))&&((*listit)->entry->portEnd>=ntohs(conn->dstPort))) {
			(*listit)->node->findRule(conn,rules);
		}
	}
	for(listit=notportlist.begin();listit!=notportlist.end();listit++) {
		if(((*listit)->entry->port<=ntohs(conn->dstPort))&&((*listit)->entry->portEnd>=ntohs(conn->dstPort))) 
		(*listit)->node->invalidateRule(conn,rules);
	}
	it=notportmap.find(ntohs(conn->dstPort));
	if(it!=notportmap.end()) it->second->invalidateRule(conn,rules);
}

void DstPortNode::invalidateRule(Connection* conn, set<IdsRule*>& rules)
{
	if(any!=NULL) any->invalidateRule(conn,rules);
	map<uint16_t,GenNode*>::iterator it, notit;
	it=portmap.find(ntohs(conn->dstPort));
	if(it!=portmap.end()) it->second->invalidateRule(conn,rules);
	list<PortListEntry*>::iterator listit;		
	for(listit=portlist.begin();listit!=portlist.end();listit++) {
		if(((*listit)->entry->port<=ntohs(conn->dstPort))&&((*listit)->entry->portEnd>=ntohs(conn->dstPort))) 
		(*listit)->node->invalidateRule(conn,rules);
	}
	it=notportmap.find(ntohs(conn->dstPort));
	if(it!=notportmap.end()) it->second->invalidateRule(conn,rules);
	for(listit=notportlist.begin();listit!=notportlist.end();listit++) {
		if(((*listit)->entry->port<=ntohs(conn->dstPort))&&((*listit)->entry->portEnd>=ntohs(conn->dstPort))) 
			(*listit)->node->invalidateRule(conn,rules);
	}
}

void DstPortNode::insertRule(IdsRule* rule,int depth) 
{
	int onlyNeg=1;
        list<PortEntry*>::iterator listit;
        for(listit=rule->dPort.begin();listit!=rule->dPort.end();listit++) {
		if((*listit)->notFlag==0) {
			onlyNeg=0;
			break;
		}
	}
	if(onlyNeg) {
		if(any==NULL) any=newNode(depth+1);
		any->insertRule(rule,depth+1);
	}
        for(listit=rule->dPort.begin();listit!=rule->dPort.end();listit++) {
		if((*listit)->port==0) {
			if(any==NULL) any=newNode(depth+1);
			any->insertRule(rule,depth+1);
		}
		else if((*listit)->port==(*listit)->portEnd) {//insert single port into map
			map<uint16_t,GenNode*>::iterator it;
			if((*listit)->notFlag==0) {
				it=portmap.find((*listit)->port);
				if(it==portmap.end()) portmap[(*listit)->port]=newNode(depth+1);
				portmap[(*listit)->port]->insertRule(rule,depth+1);
			}
			else {
				it=notportmap.find((*listit)->port);
				if(it==notportmap.end()) notportmap[(*listit)->port]=newNode(depth+1);
				notportmap[(*listit)->port]->insertRule(rule,depth+1);
			}
		}
		else if(((*listit)->portEnd!=0)&&((*listit)->port<(*listit)->portEnd)) {//insert port range into list
			if((*listit)->notFlag==0) {
				list<PortListEntry*>::iterator it;		
				for(it=portlist.begin();it!=portlist.end();it++) {
					if(((*listit)->port==(*it)->entry->port)&&((*listit)->portEnd==(*it)->entry->portEnd)) {
						(*it)->node->insertRule(rule,depth+1);
						break;
					}
				}
				if(it==portlist.end()) {
					PortListEntry* entry=new PortListEntry;	
					entry->entry=*listit;
					entry->node=newNode(depth+1);
					entry->node->insertRule(rule,depth+1);
					portlist.push_back(entry);
				}
			}
			else {
				list<PortListEntry*>::iterator it;		
				for(it=notportlist.begin();it!=notportlist.end();it++) {
					if(((*listit)->port==(*it)->entry->port)&&((*listit)->portEnd==(*it)->entry->portEnd)) {
						(*it)->node->insertRule(rule,depth+1);
						break;
					}
				}
				if(it==notportlist.end()) {
					PortListEntry* entry=new PortListEntry;	
					entry->entry=*listit;
					entry->node=newNode(depth+1);
					entry->node->insertRule(rule,depth+1);
					notportlist.push_back(entry);
				}
			}
			
		}
	}
}

void SrcPortNode::insertRevRule(IdsRule* rule,int depth) 
{
	int onlyNeg=1;
        list<PortEntry*>::iterator listit;
        for(listit=rule->dPort.begin();listit!=rule->dPort.end();listit++) {
		if((*listit)->notFlag==0) {
			onlyNeg=0;
			break;
		}
	}
	if(onlyNeg) {
		if(any==NULL) any=newNode(depth+1);
		any->insertRevRule(rule,depth+1);
	}
        for(listit=rule->dPort.begin();listit!=rule->dPort.end();listit++) {
		if((*listit)->port==0) {
			if(any==NULL) any=newNode(depth+1);
			any->insertRevRule(rule,depth+1);
		}
		else if((*listit)->port==(*listit)->portEnd) {//insert single port into map
			map<uint16_t,GenNode*>::iterator it;
			if((*listit)->notFlag==0) {
				it=portmap.find((*listit)->port);
				if(it==portmap.end()) portmap[(*listit)->port]=newNode(depth+1);
				portmap[(*listit)->port]->insertRevRule(rule,depth+1);
			}
			else {
				it=notportmap.find((*listit)->port);
				if(it==notportmap.end()) notportmap[(*listit)->port]=newNode(depth+1);
				notportmap[(*listit)->port]->insertRevRule(rule,depth+1);
			}
		}
		else if(((*listit)->portEnd!=0)&&((*listit)->port<(*listit)->portEnd)) {//insert port range into list
			if((*listit)->notFlag==0) {
				list<PortListEntry*>::iterator it;		
				for(it=portlist.begin();it!=portlist.end();it++) {
					if(((*listit)->port==(*it)->entry->port)&&((*listit)->portEnd==(*it)->entry->portEnd)) {
						(*it)->node->insertRevRule(rule,depth+1);
						break;
					}
				}
				if(it==portlist.end()) {
					PortListEntry* entry=new PortListEntry;	
					entry->entry=*listit;
					entry->node=newNode(depth+1);
					entry->node->insertRevRule(rule,depth+1);
					portlist.push_back(entry);
				}
			}
			else {
				list<PortListEntry*>::iterator it;		
				for(it=notportlist.begin();it!=notportlist.end();it++) {
					if(((*listit)->port==(*it)->entry->port)&&((*listit)->portEnd==(*it)->entry->portEnd)) {
						(*it)->node->insertRevRule(rule,depth+1);
						break;
					}
				}
				if(it==notportlist.end()) {
					PortListEntry* entry=new PortListEntry;	
					entry->entry=*listit;
					entry->node=newNode(depth+1);
					entry->node->insertRevRule(rule,depth+1);
					notportlist.push_back(entry);
				}
			}
			
		}
	}
}

DstPortNode::~DstPortNode() 
{
	if(any!=NULL) delete any;
	map<uint16_t,GenNode*>::iterator it;
	for(it=portmap.begin();it!=portmap.end();it++) {
		if(it->second!=NULL)	delete it->second;
	}
	for(it=notportmap.begin();it!=notportmap.end();it++) {
		if(it->second!=NULL)	delete it->second;
	}
}
SrcIpNode::SrcIpNode() : any(NULL) {}

void SrcIpNode::findRule(Connection* conn,set<IdsRule*>& rules) 
{
	map<uint32_t,GenNode*>::iterator it, notit;
	list<IpListEntry*>::iterator listit;
	list<GenNode*> nodes;
	list<GenNode*>::iterator nodesit;
	if(any!=NULL) any->findRule(conn,rules);
	for(int i=0;i<4;i++) {
		it=ipmaps[i].find(ntohl(conn->srcIP)>>(24-i*8));
		if(it!=ipmaps[i].end()) it->second->findRule(conn,rules);
	}
	for(listit=iplist.begin();listit!=iplist.end();listit++) {
		if ((((*listit)->entry->ip) >> (32-(*listit)->entry->mask)) == ((ntohl(conn->srcIP))>>(32-(*listit)->entry->mask))) {
			(*listit)->node->findRule(conn,rules);
		}
	}
	for(int i=0;i<4;i++) {
		it=notipmaps[i].find(ntohl(conn->srcIP)>>(24-i*8));
		if(it!=notipmaps[i].end()) it->second->invalidateRule(conn,rules);
	}
	for(listit=notiplist.begin();listit!=notiplist.end();listit++) {
		if ((((*listit)->entry->ip) >> (32-(*listit)->entry->mask)) == ((ntohl(conn->srcIP))>>(32-(*listit)->entry->mask))) {
			(*listit)->node->invalidateRule(conn,rules);
		}
	}
}

void SrcIpNode::invalidateRule(Connection* conn,set<IdsRule*>& rules) 
{
	map<uint32_t,GenNode*>::iterator it, notit;
	list<IpListEntry*>::iterator listit;
	list<GenNode*> nodes;
	list<GenNode*>::iterator nodesit;
	if(any!=NULL) any->invalidateRule(conn,rules);
	for(int i=0;i<4;i++) {
		it=ipmaps[i].find(ntohl(conn->srcIP)>>(24-i*8));
		if(it!=ipmaps[i].end()) it->second->invalidateRule(conn,rules);
	}
	for(listit=iplist.begin();listit!=iplist.end();listit++) {
		if ((((*listit)->entry->ip) >> (32-(*listit)->entry->mask)) == ((ntohl(conn->srcIP))>>(32-(*listit)->entry->mask))) {
			(*listit)->node->invalidateRule(conn,rules);
		}
	}
	for(int i=0;i<4;i++) {
		it=notipmaps[i].find(ntohl(conn->srcIP)>>(24-i*8));
		if(it!=notipmaps[i].end()) it->second->invalidateRule(conn,rules);
	}
	for(listit=notiplist.begin();listit!=notiplist.end();listit++) {
		if ((((*listit)->entry->ip) >> (32-(*listit)->entry->mask)) == ((ntohl(conn->srcIP))>>(32-(*listit)->entry->mask))) {
			(*listit)->node->invalidateRule(conn,rules);
		}
	}
}

void SrcIpNode::insertRule(IdsRule* rule,int depth) 
{
        list<IpEntry*>::iterator listit;
	int onlyNeg=1;
        for(listit=rule->src.begin();listit!=rule->src.end();listit++) {
		if((*listit)->notFlag==0) {
			onlyNeg=0;
			break;
		}
	}
	if(onlyNeg) {
		if(any==NULL) any=newNode(depth+1);
		any->insertRule(rule,depth+1);
	}
        for(listit=rule->src.begin();listit!=rule->src.end();listit++) {
		if((*listit)->mask==0) {
			if(any==NULL) any=newNode(depth+1);
			any->insertRule(rule,depth+1);
		}
		else {
			map<uint32_t,GenNode*>::iterator it;
			int fact=((*listit)->mask-1)/8;
			int mod=(*listit)->mask%8;
			if((*listit)->notFlag==0) {
				if(mod==0) {
					it=ipmaps[fact].find((*listit)->ip>>(24-fact*8));
					if(it==ipmaps[fact].end()) ipmaps[fact][(*listit)->ip>>(24-fact*8)]=newNode(depth+1);
					ipmaps[fact][(*listit)->ip>>(24-fact*8)]->insertRule(rule,depth+1);
				}
				else if(8-mod<=3) {
					for(uint32_t i=((*listit)->ip)>>(24-fact*8);(i>>(8-mod))==(((*listit)->ip)>>(32-(*listit)->mask));i++) {
						it=ipmaps[fact].find(i);
						if(it==ipmaps[fact].end()) ipmaps[fact][i]=newNode(depth+1);
						ipmaps[fact][i]->insertRule(rule,depth+1);
					}
				}
				else {
					list<IpListEntry*>::iterator it;		
					for(it=iplist.begin();it!=iplist.end();it++) {
						if(((*listit)->ip==(*it)->entry->ip)&&((*listit)->mask==(*it)->entry->mask)) {
							(*it)->node->insertRule(rule,depth+1);
							break;
						}
					}
					if(it==iplist.end()) {
						IpListEntry* entry=new IpListEntry;	
						entry->entry=*listit;
						entry->node=newNode(depth+1);
						entry->node->insertRule(rule,depth+1);
						iplist.push_back(entry);
					}
				}
			}
			else {
				if(mod==0) {
					it=notipmaps[fact].find((*listit)->ip>>(24-fact*8));
					if(it==notipmaps[fact].end()) notipmaps[fact][(*listit)->ip>>(24-fact*8)]=newNode(depth+1);
					notipmaps[fact][(*listit)->ip>>(24-fact*8)]->insertRule(rule,depth+1);
				}
				else if(8-mod<=3) {
					for(uint32_t i=((*listit)->ip)>>(24-fact*8);(i>>(8-mod))==(((*listit)->ip)>>(32-(*listit)->mask));i++) {
						it=notipmaps[fact].find(i);
						if(it==notipmaps[fact].end()) notipmaps[fact][i]=newNode(depth+1);
						notipmaps[fact][i]->insertRule(rule,depth+1);
					}
				}
				else {
					list<IpListEntry*>::iterator it;		
					for(it=notiplist.begin();it!=notiplist.end();it++) {
						if(((*listit)->ip==(*it)->entry->ip)&&((*listit)->mask==(*it)->entry->mask)) {
							(*it)->node->insertRule(rule,depth+1);
							break;
						}
					}
					if(it==notiplist.end()) {
						IpListEntry* entry=new IpListEntry;	
						entry->entry=*listit;
						entry->node=newNode(depth+1);
						entry->node->insertRule(rule,depth+1);
						notiplist.push_back(entry);
					}
				}
			}
		}
        }
}

void DstIpNode::insertRevRule(IdsRule* rule,int depth) 
{
        list<IpEntry*>::iterator listit;
	int onlyNeg=1;
        for(listit=rule->src.begin();listit!=rule->src.end();listit++) {
		if((*listit)->notFlag==0) {
			onlyNeg=0;
			break;
		}
	}
	if(onlyNeg) {
		if(any==NULL) any=newNode(depth+1);
		any->insertRevRule(rule,depth+1);
	}
        for(listit=rule->src.begin();listit!=rule->src.end();listit++) {
		if((*listit)->mask==0) {
			if(any==NULL) any=newNode(depth+1);
			any->insertRevRule(rule,depth+1);
		}
		else {
			map<uint32_t,GenNode*>::iterator it;
			int fact=((*listit)->mask-1)/8;
			int mod=(*listit)->mask%8;
			if((*listit)->notFlag==0) {
				if(mod==0) {
					it=ipmaps[fact].find((*listit)->ip>>(24-fact*8));
					if(it==ipmaps[fact].end()) ipmaps[fact][(*listit)->ip>>(24-fact*8)]=newNode(depth+1);
					ipmaps[fact][(*listit)->ip>>(24-fact*8)]->insertRevRule(rule,depth+1);
				}
				else if(8-mod<=3) {
					for(uint32_t i=((*listit)->ip)>>(24-fact*8);(i>>(8-mod))==(((*listit)->ip)>>(32-(*listit)->mask));i++) {
						it=ipmaps[fact].find(i);
						if(it==ipmaps[fact].end()) ipmaps[fact][i]=newNode(depth+1);
						ipmaps[fact][i]->insertRevRule(rule,depth+1);
					}
				}
				else {
					list<IpListEntry*>::iterator it;		
					for(it=iplist.begin();it!=iplist.end();it++) {
						if(((*listit)->ip==(*it)->entry->ip)&&((*listit)->mask==(*it)->entry->mask)) {
							(*it)->node->insertRevRule(rule,depth+1);
							break;
						}
					}
					if(it==iplist.end()) {
						IpListEntry* entry=new IpListEntry;	
						entry->entry=*listit;
						entry->node=newNode(depth+1);
						entry->node->insertRevRule(rule,depth+1);
						iplist.push_back(entry);
					}
				}
			}
			else {
				if(mod==0) {
					it=notipmaps[fact].find((*listit)->ip>>(24-fact*8));
					if(it==notipmaps[fact].end()) notipmaps[fact][(*listit)->ip>>(24-fact*8)]=newNode(depth+1);
					notipmaps[fact][(*listit)->ip>>(24-fact*8)]->insertRevRule(rule,depth+1);
				}
				else if(8-mod<=3) {
					for(uint32_t i=((*listit)->ip)>>(24-fact*8);(i>>(8-mod))==(((*listit)->ip)>>(32-(*listit)->mask));i++) {
						it=notipmaps[fact].find(i);
						if(it==notipmaps[fact].end()) notipmaps[fact][i]=newNode(depth+1);
						notipmaps[fact][i]->insertRevRule(rule,depth+1);
					}
				}
				else {
					list<IpListEntry*>::iterator it;		
					for(it=notiplist.begin();it!=notiplist.end();it++) {
						if(((*listit)->ip==(*it)->entry->ip)&&((*listit)->mask==(*it)->entry->mask)) {
							(*it)->node->insertRevRule(rule,depth+1);
							break;
						}
					}
					if(it==notiplist.end()) {
						IpListEntry* entry=new IpListEntry;	
						entry->entry=*listit;
						entry->node=newNode(depth+1);
						entry->node->insertRevRule(rule,depth+1);
						notiplist.push_back(entry);
					}
				}
			}
		}
        }
}

SrcIpNode::~SrcIpNode() 
{
	if(any!=NULL) delete any;
	map<uint32_t,GenNode*>::iterator it;
	list<IpListEntry*>::iterator listit;
	for(int i=0;i<4;i++) {
		for(it=ipmaps[i].begin();it!=ipmaps[i].end();it++) {
			if(it->second!=NULL)	delete it->second;
		}
	}
	for(int i=0;i<4;i++) {
		for(it=notipmaps[i].begin();it!=notipmaps[i].end();it++) {
			if(it->second!=NULL)	delete it->second;
		}
	}
	for(listit=iplist.begin();listit!=iplist.end();listit++) {
		delete (*listit)->node;
		delete (*listit);
	}
	for(listit=notiplist.begin();listit!=notiplist.end();listit++) {
		delete (*listit)->node;
		delete (*listit);
	}
}
DstIpNode::DstIpNode() : any(NULL) {}

void DstIpNode::findRule(Connection* conn,set<IdsRule*>& rules) 
{
	map<uint32_t,GenNode*>::iterator it, notit;
	list<IpListEntry*>::iterator listit;
	list<GenNode*> nodes;
	list<GenNode*>::iterator nodesit;
	if(any!=NULL) any->findRule(conn,rules);
	for(int i=0;i<4;i++) {
		it=ipmaps[i].find(ntohl(conn->dstIP)>>(24-i*8));
		if(it!=ipmaps[i].end()) it->second->findRule(conn,rules);
	}
	for(listit=iplist.begin();listit!=iplist.end();listit++) {
		if ((((*listit)->entry->ip) >> (32-(*listit)->entry->mask)) == ((ntohl(conn->dstIP))>>(32-(*listit)->entry->mask))) {
			(*listit)->node->findRule(conn,rules);
		}
	}
	for(int i=0;i<4;i++) {
		it=notipmaps[i].find(ntohl(conn->dstIP)>>(24-i*8));
		if(it!=notipmaps[i].end()) it->second->invalidateRule(conn,rules);
	}
	for(listit=notiplist.begin();listit!=notiplist.end();listit++) {
		if ((((*listit)->entry->ip) >> (32-(*listit)->entry->mask)) == ((ntohl(conn->dstIP))>>(32-(*listit)->entry->mask))) {
			(*listit)->node->invalidateRule(conn,rules);
		}
	}
}

void DstIpNode::invalidateRule(Connection* conn,set<IdsRule*>& rules) 
{
	map<uint32_t,GenNode*>::iterator it, notit;
	list<IpListEntry*>::iterator listit;
	list<GenNode*> nodes;
	list<GenNode*>::iterator nodesit;
	if(any!=NULL) any->invalidateRule(conn,rules);
	for(int i=0;i<4;i++) {
		it=ipmaps[i].find(ntohl(conn->dstIP)>>(24-i*8));
		if(it!=ipmaps[i].end()) it->second->invalidateRule(conn,rules);
	}
	for(listit=iplist.begin();listit!=iplist.end();listit++) {
		if ((((*listit)->entry->ip) >> (32-(*listit)->entry->mask)) == ((ntohl(conn->dstIP))>>(32-(*listit)->entry->mask))) {
			(*listit)->node->invalidateRule(conn,rules);
		}
	}
	for(int i=0;i<4;i++) {
		it=notipmaps[i].find(ntohl(conn->dstIP)>>(24-i*8));
		if(it!=notipmaps[i].end()) it->second->invalidateRule(conn,rules);
	}
	for(listit=notiplist.begin();listit!=notiplist.end();listit++) {
		if ((((*listit)->entry->ip) >> (32-(*listit)->entry->mask)) == ((ntohl(conn->dstIP))>>(32-(*listit)->entry->mask))) {
			(*listit)->node->invalidateRule(conn,rules);
		}
	}
}

void DstIpNode::insertRule(IdsRule* rule,int depth) 
{
        list<IpEntry*>::iterator listit;
	int onlyNeg=1;
        for(listit=rule->dst.begin();listit!=rule->dst.end();listit++) {
		if((*listit)->notFlag==0) {
			onlyNeg=0;
			break;
		}
	}
	if(onlyNeg) {
		if(any==NULL) any=newNode(depth+1);
		any->insertRule(rule,depth+1);
	}
        for(listit=rule->dst.begin();listit!=rule->dst.end();listit++) {
		if((*listit)->mask==0) {
			if(any==NULL) any=newNode(depth+1);
			any->insertRule(rule,depth+1);
		}
		else {
			map<uint32_t,GenNode*>::iterator it;
			int fact=((*listit)->mask-1)/8;
			int mod=(*listit)->mask%8;
			if((*listit)->notFlag==0) {
				if(mod==0) {
					it=ipmaps[fact].find((*listit)->ip>>(24-fact*8));
					if(it==ipmaps[fact].end()) ipmaps[fact][(*listit)->ip>>(24-fact*8)]=newNode(depth+1);
					ipmaps[fact][(*listit)->ip>>(24-fact*8)]->insertRule(rule,depth+1);
				}
				else if(8-mod<=3) {
					for(uint32_t i=((*listit)->ip)>>(24-fact*8);(i>>(8-mod))==(((*listit)->ip)>>(32-(*listit)->mask));i++) {
						it=ipmaps[fact].find(i);
						if(it==ipmaps[fact].end()) ipmaps[fact][i]=newNode(depth+1);
						ipmaps[fact][i]->insertRule(rule,depth+1);
					}
				}
				else {
					list<IpListEntry*>::iterator it;		
					for(it=iplist.begin();it!=iplist.end();it++) {
						if(((*listit)->ip==(*it)->entry->ip)&&((*listit)->mask==(*it)->entry->mask)) {
							(*it)->node->insertRule(rule,depth+1);
							break;
						}
					}
					if(it==iplist.end()) {
						IpListEntry* entry=new IpListEntry;	
						entry->entry=*listit;
						entry->node=newNode(depth+1);
						entry->node->insertRule(rule,depth+1);
						iplist.push_back(entry);
					}
				}
			}
			else {
				if(mod==0) {
					it=notipmaps[fact].find((*listit)->ip>>(24-fact*8));
					if(it==notipmaps[fact].end()) notipmaps[fact][(*listit)->ip>>(24-fact*8)]=newNode(depth+1);
					notipmaps[fact][(*listit)->ip>>(24-fact*8)]->insertRule(rule,depth+1);
				}
				else if(8-mod<=3) {
					for(uint32_t i=((*listit)->ip)>>(24-fact*8);(i>>(8-mod))==(((*listit)->ip)>>(32-(*listit)->mask));i++) {
						it=notipmaps[fact].find(i);
						if(it==notipmaps[fact].end()) notipmaps[fact][i]=newNode(depth+1);
						notipmaps[fact][i]->insertRule(rule,depth+1);
					}
				}
				else {
					list<IpListEntry*>::iterator it;		
					for(it=notiplist.begin();it!=notiplist.end();it++) {
						if(((*listit)->ip==(*it)->entry->ip)&&((*listit)->mask==(*it)->entry->mask)) {
							(*it)->node->insertRule(rule,depth+1);
							break;
						}
					}
					if(it==notiplist.end()) {
						IpListEntry* entry=new IpListEntry;	
						entry->entry=*listit;
						entry->node=newNode(depth+1);
						entry->node->insertRule(rule,depth+1);
						notiplist.push_back(entry);
					}
				}
			}
		}
        }
}

void SrcIpNode::insertRevRule(IdsRule* rule,int depth) 
{
        list<IpEntry*>::iterator listit;
	int onlyNeg=1;
        for(listit=rule->dst.begin();listit!=rule->dst.end();listit++) {
		if((*listit)->notFlag==0) {
			onlyNeg=0;
			break;
		}
	}
	if(onlyNeg) {
		if(any==NULL) any=newNode(depth+1);
		any->insertRevRule(rule,depth+1);
	}
        for(listit=rule->dst.begin();listit!=rule->dst.end();listit++) {
		if((*listit)->mask==0) {
			if(any==NULL) any=newNode(depth+1);
			any->insertRevRule(rule,depth+1);
		}
		else {
			map<uint32_t,GenNode*>::iterator it;
			int fact=((*listit)->mask-1)/8;
			int mod=(*listit)->mask%8;
			if((*listit)->notFlag==0) {
				if(mod==0) {
					it=ipmaps[fact].find((*listit)->ip>>(24-fact*8));
					if(it==ipmaps[fact].end()) ipmaps[fact][(*listit)->ip>>(24-fact*8)]=newNode(depth+1);
					ipmaps[fact][(*listit)->ip>>(24-fact*8)]->insertRevRule(rule,depth+1);
				}
				else if(8-mod<=3) {
					for(uint32_t i=((*listit)->ip)>>(24-fact*8);(i>>(8-mod))==(((*listit)->ip)>>(32-(*listit)->mask));i++) {
						it=ipmaps[fact].find(i);
						if(it==ipmaps[fact].end()) ipmaps[fact][i]=newNode(depth+1);
						ipmaps[fact][i]->insertRevRule(rule,depth+1);
					}
				}
				else {
					list<IpListEntry*>::iterator it;		
					for(it=iplist.begin();it!=iplist.end();it++) {
						if(((*listit)->ip==(*it)->entry->ip)&&((*listit)->mask==(*it)->entry->mask)) {
							(*it)->node->insertRevRule(rule,depth+1);
							break;
						}
					}
					if(it==iplist.end()) {
						IpListEntry* entry=new IpListEntry;	
						entry->entry=*listit;
						entry->node=newNode(depth+1);
						entry->node->insertRevRule(rule,depth+1);
						iplist.push_back(entry);
					}
				}
			}
			else {
				if(mod==0) {
					it=notipmaps[fact].find((*listit)->ip>>(24-fact*8));
					if(it==notipmaps[fact].end()) notipmaps[fact][(*listit)->ip>>(24-fact*8)]=newNode(depth+1);
					notipmaps[fact][(*listit)->ip>>(24-fact*8)]->insertRevRule(rule,depth+1);
				}
				else if(8-mod<=3) {
					for(uint32_t i=((*listit)->ip)>>(24-fact*8);(i>>(8-mod))==(((*listit)->ip)>>(32-(*listit)->mask));i++) {
						it=notipmaps[fact].find(i);
						if(it==notipmaps[fact].end()) notipmaps[fact][i]=newNode(depth+1);
						notipmaps[fact][i]->insertRevRule(rule,depth+1);
					}
				}
				else {
					list<IpListEntry*>::iterator it;		
					for(it=notiplist.begin();it!=notiplist.end();it++) {
						if(((*listit)->ip==(*it)->entry->ip)&&((*listit)->mask==(*it)->entry->mask)) {
							(*it)->node->insertRevRule(rule,depth+1);
							break;
						}
					}
					if(it==notiplist.end()) {
						IpListEntry* entry=new IpListEntry;	
						entry->entry=*listit;
						entry->node=newNode(depth+1);
						entry->node->insertRevRule(rule,depth+1);
						notiplist.push_back(entry);
					}
				}
			}
		}
        }
}

DstIpNode::~DstIpNode() 
{
	map<uint32_t,GenNode*>::iterator it;
	list<IpListEntry*>::iterator listit;
	if(any!=NULL) delete any;
	for(int i=0;i<4;i++) {
		for(it=ipmaps[i].begin();it!=ipmaps[i].end();it++) {
			if(it->second!=NULL)	delete it->second;
		}
	}
	for(int i=0;i<4;i++) {
		for(it=notipmaps[i].begin();it!=notipmaps[i].end();it++) {
			if(it->second!=NULL)	delete it->second;
		}
	}
	for(listit=iplist.begin();listit!=iplist.end();listit++) {
		delete (*listit)->node;
		delete (*listit);
	}
	for(listit=notiplist.begin();listit!=notiplist.end();listit++) {
		delete (*listit)->node;
		delete (*listit);
	}
}



void RuleNode::insertRule(IdsRule* rule,int depth) 
{
	rulesList.push_back(rule);
}

void RuleNode::insertRevRule(IdsRule* rule,int depth) 
{
	rulesList.push_back(rule);
}

void RuleNode::findRule(Connection* conn,set<IdsRule*>& rules) 
{
	list<IdsRule*>::iterator it;
	for(it=rulesList.begin();it!=rulesList.end();it++) {
		rules.insert(*it);
	}
}

void RuleNode::invalidateRule(Connection* conn, set<IdsRule*>& rules) 
{
	list<IdsRule*>::iterator it;
	for(it=rulesList.begin();it!=rulesList.end();it++) {
		rules.erase(*it);
	}
}

RuleNode::~RuleNode(){}

GenNode* GenNode::newNode(int depth) 
{
        if(order[depth]==0) return new ProtoNode;
	else if(order[depth]==1) return new SrcIpNode;
	else if(order[depth]==2) return new DstIpNode;
	else if(order[depth]==3) return new SrcPortNode;
	else if(order[depth]==4) return new DstPortNode;
	else return new RuleNode;
}

int FlowSigMatcher::parse_line(string text) 
{
	boost::cmatch what;
	const boost::regex expLine("(\\d+) +((?:!?\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}(?:/\\d+)*)|(?:\\[.+\\])|(?:!?\\$HOME_NET)|any|ANY) +(!?\\d+|any|ANY|\\*|(?:!?(?:\\d+)?\\:(?:\\d+)?)|(?:\\[.+\\])) +(->|<>|<!>|=>|<=>) +((?:!?\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}(?:/\\d+)*|any|ANY)|(?:\\[.+\\])|(?:!?\\$HOME_NET)) +(!?\\d+|any|ANY|\\*|(?:!?(?:\\d+)?\\:(?:\\d+)?)|(?:\\[.+\\])) +(\\w+) +([\\w\\-]+) +([\\w\\-]+) +\"(.*)\"(?: +(\\d+))?");
	const boost::regex expFlagsLine("(\\d+) +flags +\\((.+)\\) +([\\w\\-]+) +([\\w\\-]+) +\"(.*)\"");
	const boost::regex expCommentLine("^#|^[:space:]*$");
	if(boost::regex_search(text.c_str(), expCommentLine)) return true;
	else if(boost::regex_match(text.c_str(), what, expLine)) { 
		IdsRule* rule=new IdsRule;
		rule->uid=atoi(static_cast<string>(what[1]).c_str());
		if((!parse_ip(what[2], *rule,0))||(!parse_port(what[3],*rule,0))||(!parse_ip(what[5], *rule,1))||(parse_port(what[6],*rule,1)==false)) {
			delete rule;
			return false;
		}
		if(static_cast<string>(what[7]).compare("TCP")==0||static_cast<string>(what[7]).compare("tcp")==0) rule->protocol=6;
		else if(static_cast<string>(what[7]).compare("UDP")==0||static_cast<string>(what[7]).compare("udp")==0) rule->protocol=17;
		else if(static_cast<string>(what[7]).compare("ICMP")==0||static_cast<string>(what[7]).compare("icmp")==0) rule->protocol=1;
		else rule->protocol=0;
		string dir(what[4]);
		if(dir.compare("->")==0) {
			rule->mode=0;
		} else if(dir.compare("<>")==0) {
			rule->mode=1;
		} else if(dir.compare("<!>")==0) {
			rule->mode=2;
		} else if((dir.compare("=>")==0)&&(rule->protocol==6)) {
			rule->mode=3;
		} else if((dir.compare("<=>")==0)&&(rule->protocol==6)) {
			rule->mode=4;
                } else {
			delete rule;
			return false;
		}
		rule->type=findVectorNr(what[8],idsRuleType);
		rule->source=findVectorNr(what[9],idsRuleSource);
		rule->msg=what[10];
		if(static_cast<string>(what[11]).compare("")==0) rule->flag=0;
		else rule->flag=atoi(static_cast<string>(what[11]).c_str());
		parsedRules.push_back(rule);
		return true;
	}
	else if(boost::regex_match(text.c_str(), what, expFlagsLine)) { 
		FlagsRule* rule= new FlagsRule();
		flagRules.push_back(rule);
		parseFlags(what[2],*rule);
		rule->uid=atoi(static_cast<string>(what[1]).c_str());
		rule->type=findVectorNr(what[3],idsRuleType);
		rule->source=findVectorNr(what[4],idsRuleSource);
		rule->msg=what[5];
		return true;
	}
	else return false;
}

uint8_t FlowSigMatcher::findVectorNr(string text, vector<string>& vec) 
{
	uint8_t i;
	for(i=0;i<vec.size();i++) {
		if(vec[i].compare(text)==0) return i;
	}
	if(vec.size()<256) vec.push_back(text);
        else THROWEXCEPTION("maximal number of different Sources or Types is reached");
	return i;
}

int FlowSigMatcher::parse_port(string text, IdsRule& rule, uint32_t dst)
{
	const boost::regex exp_braces("\\[.+\\]");
	if(boost::regex_match(text,exp_braces)) {
		text.erase(text.begin());
		text.erase(text.end()-1);
		const boost::regex expIp(", *| +");
		boost::sregex_token_iterator i(text.begin(), text.end(), expIp, -1);
		boost::sregex_token_iterator j;
		if(dst==0)  while(i!=j) split_port(*i++,rule.sPort);
		else while(i!=j) split_port(*i++,rule.dPort);
	}
	else {
		if(dst==0) split_port(text,rule.sPort);
		else split_port(text,rule.dPort);
	}
	if(dst==0) {
		if(rule.sPort.size()==0) return false;
	}
	else {
		if(rule.dPort.size()==0) return false;
	}
	return true;
} 

void FlowSigMatcher::split_port(string text, list<PortEntry*>& list) 
{
	boost::cmatch what;
	const boost::regex exp_port_range("(!)?(\\d+)?\\:(\\d+)?");
	const boost::regex exp_port("(!)?(\\d+)");
	const boost::regex exp_anyport("(\\*|any|ANY)");
	PortEntry* entry;
	if(boost::regex_match(text.c_str(), what, exp_port_range)) {
		entry= new PortEntry;
		if(static_cast<string>(what[1]).compare("!")==0) entry->notFlag=1; 
		else entry->notFlag=0;
		entry->port=atoi(static_cast<string>(what[2]).c_str());
		if(static_cast<string>(what[2]).compare("")==0) entry->port=1;
		if(static_cast<string>(what[3]).compare("")==0) entry->portEnd=65535;
		else entry->portEnd=atoi(static_cast<string>(what[3]).c_str());
		if(entry->port>entry->portEnd) {
			delete entry;
			return;
		}
	}
	else if(boost::regex_match(text.c_str(), what, exp_port)) {
		entry= new PortEntry;
		if (static_cast<string>(what[1]).compare("!")==0) entry->notFlag=1; 
		else entry->notFlag=0;
		entry->port=atoi(static_cast<string>(what[2]).c_str());
		entry->portEnd=entry->port;
	}
	else if(boost::regex_match(text.c_str(),what,exp_anyport)) {
		entry = new PortEntry;
		entry->notFlag=0;
		entry->port=0;
	}
	else {
		msg(MSG_DIALOG,"Couldn't parse this port parameter: %s",text.c_str());
		return;
	}
	list.push_back(entry);
}

int FlowSigMatcher::parseFlags(string text, FlagsRule& rule) 
{
	const boost::regex expFlags(", *| +");
	boost::sregex_token_iterator i(text.begin(), text.end(), expFlags, -1);
	boost::sregex_token_iterator j;
	while(i!=j) {
		rule.flags.insert(atoi(static_cast<string>(*i++).c_str()));
	}
	return true;
}

int FlowSigMatcher::parse_ip(string text, IdsRule& rule, uint32_t dst) 
{
	const boost::regex exp_braces("\\[.+\\]");
	if(boost::regex_match(text,exp_braces)) {
		text.erase(text.begin());
		text.erase(text.end()-1);
		const boost::regex expIp(", *| +");
		boost::sregex_token_iterator i(text.begin(), text.end(), expIp, -1);
		boost::sregex_token_iterator j;
		if(dst==0)  while(i!=j) split_ip(*i++,rule.src);
		else while(i!=j) split_ip(*i++,rule.dst);
	}
	else {
		if(dst==0) split_ip(text,rule.src);
		else split_ip(text,rule.dst);
	}
	if(dst==0) {
		if(rule.src.size()==0) return false;
	}
	else {
		if(rule.dst.size()==0) return false;
	}
	return true;
}

void FlowSigMatcher::split_ip(string text, list<IpEntry*>& list) 
{
	boost::cmatch what;
	const boost::regex exp_split_ip("(!)?(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})(?:/(\\d+))*");
	const boost::regex exp_split_home("(!)?(\\$HOME_NET)");
	const boost::regex exp_split_anyip("(any|ANY)");
	if(boost::regex_match(text.c_str(),what,exp_split_ip)) {
		IpEntry* entry=new IpEntry;
		string emptymask="";
		if(static_cast<string>(what[1]).compare("!")==0)entry->notFlag=1;
		else entry->notFlag=0;
		if(emptymask.compare(what[6])==0) entry->mask=32;
		else entry->mask=atoi(static_cast<string>(what[6]).c_str());
		entry->ip=(atoi(static_cast<string>(what[2]).c_str())<<24)|(atoi(static_cast<string>(what[3]).c_str())<<16)|(atoi(static_cast<string>(what[4]).c_str())<<8)|(atoi(static_cast<string>(what[5]).c_str()));
		list.push_back(entry);
	}
	else if(boost::regex_match(text.c_str(),what,exp_split_anyip)) {
		IpEntry* entry=new IpEntry;
		entry->mask=0;
		entry->ip=0;
		list.push_back(entry);
	}
	else if(boost::regex_match(text.c_str(),what,exp_split_home)) {
		string text=homenet;
		const boost::regex exp_braces("\\[.+\\]");
		if(boost::regex_match(text,exp_braces)) {
			text.erase(text.begin());
			text.erase(text.end()-1);
			const boost::regex expIp(", *| +");
			boost::sregex_token_iterator i(text.begin(), text.end(), expIp, -1);
			boost::sregex_token_iterator j;
			if(static_cast<string>(what[1]).compare("")==0) {
				while(i!=j) {
					split_ip(*i++,list);
				}
			}
			else {
				while(i!=j) {
					string tmp=*i++;
					if(tmp[0]!='!')	split_ip('!'+ tmp,list);
					else {
						tmp.erase(0,1);
						split_ip(tmp,list);
					}
				}
			}
		}
		else {
			if(static_cast<string>(what[1]).compare("")==0) split_ip(homenet,list);
			else {
				string tmp=homenet;
				if(tmp[0]!='!') split_ip('!'+tmp,list);
				else {
					tmp.erase(0,1);
					split_ip(tmp,list);
				}
			}
		}
	}
	else {
		msg(MSG_DIALOG,"Couldn't parse this IP parameter: %s",text.c_str());
	}
}
