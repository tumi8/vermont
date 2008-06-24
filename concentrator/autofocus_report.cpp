#include "autofocus_report.h"
#include "autofocus_attribute.h"
#include "autofocus_iprecord.h"
#include "common/Misc.h"
#include <vector>

using namespace std;
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

void rep_payload_tcp::post(vector<treeRecord*>* currentTree,treeRecord* lastTree,uint32_t index)
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

void rep_payload_udp::post(vector<treeRecord*>* currentTree,treeRecord* lastTree,uint32_t index)
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
void rep_fanouts::post(vector<treeRecord*>*  currentTree,treeRecord* lastTree,uint32_t index)
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

void rep_fanins::post(vector<treeRecord*>*  currentTree,treeRecord* lastTree,uint32_t index)
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

void rep_packets_tcp::post(vector<treeRecord*>*  currentTree,treeRecord* lastTree,uint32_t index)
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

void rep_packets_udp::post(vector<treeRecord*>* currentTree,treeRecord* lastTree,uint32_t index)
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

void rep_failed::post(vector<treeRecord*>* currentTree,treeRecord* lastTree,uint32_t index)
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

void rep_simult::post(vector<treeRecord*>*  currentTree,treeRecord* lastTree,uint32_t index)
{
	list<treeNode*>::iterator iter = specNodes.begin();
	msg(MSG_FATAL,"Total count simult %d",numTotal);
	while (iter != specNodes.end()) 
	{

		treeNode* before = getComparismValue(*iter,*currentTree,index);
		uint64_t data = (*iter)->data.m_attributes[simult]->numCount;
		double percentage = (double) (data*100) / (double) numTotal;	

		msg(MSG_FATAL,"SUBNET: %s/%d\t	ConnectionCount: %03llu (%01.2f%%)\t",
				IPToString((*iter)->data.subnetIP).c_str(),(*iter)->data.subnetBits,
				data,percentage);

		iter++;
	}
}
//------------------------------------------------------------

treeNode* report::getComparismValue(treeNode* match,vector<treeRecord>& m_treeRecords,uint32_t index)
{
	if (m_treeRecords[index - 1 m_treeRecord.size() % m_treeRecord.size()] == NULL) return NULL;
	else 
	{
		uint32_t sip = ntohl(match->data.subnetIP);
		uint32_t sbits = match->data.subnetBits;
		treeNode* current = m_treeRecords[iindex - 1 m_treeRecord.size() % m_treeRecord.size()]->root;
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
void report::checkNode(treeNode* newnode,uint32_t numMax)
{
	int threshold = numTotal / numMax;

	if (newnode->data.m_attributes[getID()]->delta > threshold) 
	{
		newnode->data.m_attributes[getID()]->delta = 0;
		specNodes.push_back(newnode);
	}
}
