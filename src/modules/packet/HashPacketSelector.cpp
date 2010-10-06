
#include "HashPacketSelector.hpp"

HashPacketSelector::HashPacketSelector() 
{
}
HashPacketSelector::~HashPacketSelector()
{
}
	
int HashPacketSelector::decide(Packet *p)
{
	if (p->ipProtocolType == Packet::TCP || p->ipProtocolType == Packet::UDP){
		return ((
			*((uint32_t *) p->netHeader + SRC_ADDRESS_OFFSET) +
			*((uint32_t *) p->netHeader + DST_ADDRESS_OFFSET) +
			*((uint32_t *) p->transportHeader + TCP_UDP_SRC_PORT_OFFSET) +
			*((uint32_t *) p->transportHeader + TCP_UDP_DST_PORT_OFFSET) 
			) % numberOfQueues);
	}
	return ((*((uint32_t *) p->netHeader + SRC_ADDRESS_OFFSET) + *((uint32_t *) p->netHeader + DST_ADDRESS_OFFSET)) % numberOfQueues);
}

