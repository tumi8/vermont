#include "autofocus_report.h"
#include "autofocus_attribute.h"
#include "autofocus_iprecord.h"
#include "common/Misc.h"
report::report() 
{
	numTotal = 0;
}

uint64_t report::getTotal()
{
	return numTotal;
}

void report::aggregate(uint64_t p_num) 
{
	numTotal += p_num;
}
//------------------------------------------------------------

af_attribute* rep_payload_tcp::createaf_attribute()
{
	return new atr_payload_tcp(this);
}

af_attribute* rep_payload_udp::createaf_attribute()
{
	return new atr_payload_udp(this);
}

af_attribute* rep_fanouts::createaf_attribute()
{
	return new atr_fanouts(this);
}

af_attribute* rep_fanins::createaf_attribute()
{
	return new atr_fanins(this);
}

af_attribute* rep_packets_tcp::createaf_attribute()
{
	return new atr_packets_tcp(this);
}

af_attribute* rep_packets_udp::createaf_attribute()
{
	return new atr_packets_udp(this);
}

af_attribute* rep_failed::createaf_attribute()
{
	return new atr_failed(this);
}

af_attribute* rep_simult::createaf_attribute()
{
	return new atr_simult(this);
}

//------------------------------------------------------------

report_enum rep_payload_tcp::getID() 
{
	return payload_tcp;
}

report_enum rep_payload_udp::getID() 
{
	return payload_udp;
}

report_enum rep_fanouts::getID() 
{
	return fanouts;
}

report_enum rep_fanins::getID() 
{
	return fanins;
}

report_enum rep_packets_tcp::getID() 
{
	return packets_tcp;
}

report_enum rep_packets_udp::getID() 
{
	return packets_udp;
}

report_enum rep_failed::getID() 
{
	return failed;
}

report_enum rep_simult::getID() 
{
	return simult;
}
//------------------------------------------------------------

void rep_payload_tcp::post(treeRecord* currentTree,treeRecord* lastTree,uint32_t index)
{
	msg(MSG_FATAL,"Total count TCP %d",numTotal/1024/1024);

	list<treeNode*>::iterator iter = specNodes.begin();

	while (iter != specNodes.end()) 
	{

		uint64_t data = (*iter)->data.m_attributes[payload_tcp]->numCount;

		double percentage = (double) (data*100) / (double) numTotal;	

		msg(MSG_FATAL,"SUBNET: %s/%d\t\t TRAFFIC: %03llu MB (%01.2f%%)\t",
				IPToString((*iter)->data.subnetIP).c_str(),(*iter)->data.subnetBits,
				data/1024/1024,percentage);

		iter++;
	}
}

void rep_payload_udp::post(treeRecord* currentTree,treeRecord* lastTree,uint32_t index)
{

	msg(MSG_FATAL,"Total count UDP %d",numTotal/1024/1024);

	list<treeNode*>::iterator iter = specNodes.begin();

	while (iter != specNodes.end()) 
	{

		uint64_t data = (*iter)->data.m_attributes[payload_udp]->numCount;

		double percentage = (double) (data*100) / (double) numTotal;	

		msg(MSG_FATAL,"SUBNET: %s/%d\t\t TRAFFIC: %03llu MB (%01.2f%%)\t",
				IPToString((*iter)->data.subnetIP).c_str(),(*iter)->data.subnetBits,
				data/1024/1024,percentage);

		iter++;
	}

}
void rep_fanouts::post(treeRecord* currentTree,treeRecord* lastTree,uint32_t index)
{
	list<treeNode*>::iterator iter = specNodes.begin();
	msg(MSG_FATAL,"Total count FanOuts %d",numTotal);
	while (iter != specNodes.end()) 
	{

		uint64_t data = (*iter)->data.m_attributes[fanouts]->numCount;

		double percentage = (double) (data*100) / (double) numTotal;	

		msg(MSG_FATAL,"SUBNET: %s/%d\t	FANOUTS: %03llu (%01.2f%%)\t",
				IPToString((*iter)->data.subnetIP).c_str(),(*iter)->data.subnetBits,
				data,percentage);

		iter++;
	}
}

void rep_fanins::post(treeRecord* currentTree,treeRecord* lastTree,uint32_t index)
{
	list<treeNode*>::iterator iter = specNodes.begin();
	msg(MSG_FATAL,"Total count FanIns %d",numTotal);
	while (iter != specNodes.end()) 
	{

		uint64_t data = (*iter)->data.m_attributes[fanins]->numCount;

		double percentage = (double) (data*100) / (double) numTotal;	

		msg(MSG_FATAL,"SUBNET: %s/%d\t	FANOUTS: %03llu (%01.2f%%)\t",
				IPToString((*iter)->data.subnetIP).c_str(),(*iter)->data.subnetBits,
				data,percentage);

		iter++;
	}
}

void rep_packets_tcp::post(treeRecord* currentTree,treeRecord* lastTree,uint32_t index)
{
	list<treeNode*>::iterator iter = specNodes.begin();
	msg(MSG_FATAL,"Total count packets TCP %d",numTotal);
	while (iter != specNodes.end()) 
	{

		uint64_t data = (*iter)->data.m_attributes[packets_tcp]->numCount;

		double percentage = (double) (data*100) / (double) numTotal;	

		msg(MSG_FATAL,"SUBNET: %s/%d\t	PacketCount: %03llu (%01.2f%%)\t",
				IPToString((*iter)->data.subnetIP).c_str(),(*iter)->data.subnetBits,
				data,percentage);

		iter++;
	}
}

void rep_packets_udp::post(treeRecord* currentTree,treeRecord* lastTree,uint32_t index)
{
	list<treeNode*>::iterator iter = specNodes.begin();
	msg(MSG_FATAL,"Total count packets UDP %d",numTotal);
	while (iter != specNodes.end()) 
	{

		uint64_t data = (*iter)->data.m_attributes[packets_udp]->numCount;

		double percentage = (double) (data*100) / (double) numTotal;	

		msg(MSG_FATAL,"SUBNET: %s/%d\t	PacketCount: %03llu (%01.2f%%)\t",
				IPToString((*iter)->data.subnetIP).c_str(),(*iter)->data.subnetBits,
				data,percentage);

		iter++;
	}
}

void rep_failed::post(treeRecord* currentTree,treeRecord* lastTree,uint32_t index)
{
	list<treeNode*>::iterator iter = specNodes.begin();
	msg(MSG_FATAL,"Total count failed %d",numTotal);
	while (iter != specNodes.end()) 
	{

		uint64_t data = (*iter)->data.m_attributes[failed]->numCount;

		double percentage = (double) (data*100) / (double) numTotal;	

		msg(MSG_FATAL,"SUBNET: %s/%d\t	PacketCount: %03llu (%01.2f%%)\t",
				IPToString((*iter)->data.subnetIP).c_str(),(*iter)->data.subnetBits,
				data,percentage);

		iter++;
	}
}

void rep_simult::post(treeRecord* currentTree,treeRecord* lastTree,uint32_t index)
{
	list<treeNode*>::iterator iter = specNodes.begin();
	msg(MSG_FATAL,"Total count simult %d",numTotal);
	while (iter != specNodes.end()) 
	{

		uint64_t data = (*iter)->data.m_attributes[simult]->numCount;

		double percentage = (double) (data*100) / (double) numTotal;	

		msg(MSG_FATAL,"SUBNET: %s/%d\t	ConnectionCount: %03llu (%01.2f%%)\t",
				IPToString((*iter)->data.subnetIP).c_str(),(*iter)->data.subnetBits,
				data,percentage);

		iter++;
	}
}
//------------------------------------------------------------

void report::checkNode(treeNode* newnode,uint32_t numMax)
{
	int threshold = numTotal / numMax;

	if (newnode->data.m_attributes[getID()]->delta > threshold) 
	{
		newnode->data.m_attributes[getID()]->delta = 0;
		specNodes.push_back(newnode);
	}
}
