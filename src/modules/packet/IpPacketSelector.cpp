#include "IpPacketSelector.hpp"

IpPacketSelector::IpPacketSelector() 
{
}
IpPacketSelector::~IpPacketSelector()
{
}
void IpPacketSelector::initializeConfig(std::map<uint32_t, int>& src, std::map<uint32_t, int>& dst){
	srcips = src;
	dstips = dst;
}
	
void IpPacketSelector::addDestinationIp(uint32_t dst, int queueno)
{
	dstips[dst] = queueno;
}

void IpPacketSelector::addSourceIp(uint32_t src, int queueno)
{
	srcips[src] = queueno;
}

int IpPacketSelector::decide(Packet *p)
{
	//msg(MSG_INFO, "decider: %d, %d", srcips.size(), dstips.size());
	uint32_t src = 	*((uint32_t *)(p->netHeader + SRC_ADDRESS_OFFSET));
	uint32_t dst = 	*((uint32_t *)(p->netHeader + DST_ADDRESS_OFFSET));
	
	if(srcips.find(src) != srcips.end())
		return srcips[src];
	if(dstips.find(dst) != dstips.end())
		return dstips[dst];
	return -1;

}
