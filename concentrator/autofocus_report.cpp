#include "autofocus_report.h"
#include "autofocus_attribute.h"

report::report() 
{
	numTotal = 0;
}

report::getTotal()
{
	return numTotal();
}

report::aggregate(uint64_t p_num) 
{
	numTotal += p_num;
}

rep_payload_tcp::createAttribute()
{
	return new atr_payload_tcp();
}

rep_payload_udp::createAttribute()
{
	return new atr_payload_udp();
}
rep_payload_tcp::createAttribute()
{
	return new atr_fanouts();
}

rep_fanouts::post(treeRecord* currentTree,treeRecord* lastTree,uint32_t index)
{
	list<treeNode*>::iterator iter = >specNodes.begin();

	while (iter != currentTree->specNodes.end()) 
	{

		double percentage = 0;
		treeNode* last_value = getComparismValue(*iter,index-1);


		percentage = (double) ((*iter)->data.fanouts*100) / (double) numTotal;	


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

}
rep_payload_tcp::post()
{


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


}
