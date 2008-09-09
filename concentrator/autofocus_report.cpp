#include "autofocus_report.h"
#include "autofocus_attribute.h"
#include "autofocus_iprecord.h"
#include "common/Misc.h"
#include <vector>
#include <math.h>
#include <iostream>

using namespace std;
report::report(uint32_t a) 
{
	numTotal = 0;
	minSubbits = a;
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

void rep_payload_tcp::post(vector<treeRecord*>& p_treeRecords,uint32_t index)
{
	f_post(p_treeRecords,index,payload_tcp);
}

void rep_payload_udp::post(vector<treeRecord*>& p_treeRecords,uint32_t index)
{
	f_post(p_treeRecords,index,payload_udp);
}
void rep_fanouts::post(vector<treeRecord*>& p_treeRecords,uint32_t index)
{
	f_post(p_treeRecords,index,fanouts);
}

void rep_fanins::post(vector<treeRecord*>& p_treeRecords,uint32_t index)
{
	f_post(p_treeRecords,index,fanins);
}

void rep_packets_tcp::post(vector<treeRecord*>& p_treeRecords,uint32_t index)
{
	f_post(p_treeRecords,index,packets_tcp);
}

void rep_packets_udp::post(vector<treeRecord*>& p_treeRecords,uint32_t index)
{
	f_post(p_treeRecords,index,packets_udp);
}

void rep_failed::post(vector<treeRecord*>& p_treeRecords,uint32_t index)
{
	f_post(p_treeRecords,index,failed);
}

void rep_simult::post(vector<treeRecord*>& p_treeRecords,uint32_t index)
{
	f_post(p_treeRecords,index,simult);
}
//-----------------------------------------------------------

/*
 * Print out all nodes that are reportworthy. 
 * printing to stdout is not enabled, instead save hosts in a list so a metalist can be created later
 *
 */
void report::f_post(vector<treeRecord*>& p_treeRecords,uint32_t index,report_enum attribute)
{

	list<treeNode*>::iterator iter = specNodes.begin();

	uint32_t lastindex = (index - 1 + p_treeRecords.capacity()) % p_treeRecords.capacity();
	double change_global;
	if (p_treeRecords[lastindex] != NULL) 
	{
		change_global = (double) (numTotal * 100) / (double) p_treeRecords[lastindex]->root->data.m_attributes[attribute]->numCount - 100.0;	
	//	msg(MSG_FATAL,"Total %s %03llu CHANGE: %01.2f%%",global.c_str(),numTotal,change_global);
	}
	else {
	//	msg(MSG_FATAL,"Total %s %d",global.c_str(),numTotal);
	}

	while (iter != specNodes.end()) 
	{

		//msg(MSG_FATAL,"SUBNET: %s/%d\t\t %s\t",IPToString((*iter)->data.subnetIP).c_str(),(*iter)->data.subnetBits, global.c_str());

		if ((*iter)->data.subnetBits < minSubbits)
		{ 
			iter = specNodes.erase(iter);
			continue;
		}

		treeNode* before = getComparismValue(*iter,p_treeRecords,index);

		uint64_t data = (*iter)->data.m_attributes[attribute]->numCount;

		if (before != NULL)
		{

			double change = (double) (data * 100) / (double) before->data.m_attributes[attribute]->numCount - 100.0;	

			if (abs((int) (change_global - change)) < THRESHOLD) 
			{
				iter = specNodes.erase(iter);
				continue;
			}

			(*iter)->prio += abs((int)change);
		//msg(MSG_FATAL,"prio SUBNET: %s/%d\t\t %s\t %d",IPToString(before->data.subnetIP).c_str(),before->data.subnetBits,local.c_str(),abs((int)change));
//		msg(MSG_FATAL,"SUBNET: %s/%d\t\t %s\t %03llu (%03llu)\t\t %01.2f%% (%01.2f%%)\t",IPToString(before->data.subnetIP).c_str(),before->data.subnetBits,local.c_str(),data,before->data.m_attributes[attribute]->numCount,percentage,change);
		}
		else 
		{
//			msg(MSG_FATAL,"SUBNET: %s/%d\t	%s: %03llu (%01.2f%%)\t",			IPToString((*iter)->data.subnetIP).c_str(),(*iter)->data.subnetBits,				local.c_str(),data,percentage);
		}
		iter++;
	}

}


//-----------------------------------------------------------


/*
 * This function provides the possibility to compare a host's value to the one in the last tree
 *
 *
 */
treeNode* report::getComparismValue(treeNode* match,vector<treeRecord*>& m_treeRecords,uint32_t index)
{
	uint32_t lastindex = (index - 1 + m_treeRecords.capacity()) % m_treeRecords.capacity();

	if (m_treeRecords[lastindex] == NULL) { return NULL; }
	else 
	{
		uint32_t sbits = match->data.subnetBits;
		treeNode* current = m_treeRecords[lastindex]->root;
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
/*
 * check if a node exceeds its threshold
 *
 */
void report::checkNode(treeNode* newnode,uint32_t numMax)
{
	uint32_t threshold = numTotal / numMax;

	if (newnode->data.m_attributes[getID()]->delta >= threshold) 
	{
		newnode->data.m_attributes[getID()]->delta = 0;
		specNodes.push_back(newnode);
	}
}

uint32_t report::distance(treeNode* a,treeNode* b) 
{
	return ntohl(a->data.subnetIP) ^ ntohl(b->data.subnetIP);
}
