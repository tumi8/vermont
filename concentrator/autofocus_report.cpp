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

af_attribute* rep_payload_tcp::createaf_attribute()
{
	return new atr_payload_tcp(this);
}
report_enum rep_payload_tcp::getID() 
{
	return payload_tcp;
}

af_attribute* rep_payload_udp::createaf_attribute()
{
	return new atr_payload_udp(this);
}

report_enum rep_payload_udp::getID() 
{
	return payload_udp;
}

af_attribute* rep_fanouts::createaf_attribute()
{
	return new atr_fanouts(this);
}
report_enum rep_fanouts::getID() 
{
	return fanouts;
}

void rep_fanouts::post(treeRecord* currentTree,treeRecord* lastTree,uint32_t index)
{
	list<treeNode*>::iterator iter = specNodes.begin();

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


void rep_payload_tcp::post(treeRecord* currentTree,treeRecord* lastTree,uint32_t index)
{

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


}

void report::checkNode(treeNode* newnode,uint32_t numMax)
{
	int threshold = numTotal / numMax;

	if (newnode->data.m_attributes[getID()]->delta > threshold) 
	{
		newnode->data.m_attributes[getID()]->delta = 0;
		specNodes.push_back(newnode);
	}
}
