#include "reconf/FilterModule.h"

FilterModule::FilterModule()
{
}

/*
 * this is the main filter function: each packet is run thru a series of 
 * PacketProcessors and then pushed to the next PacketSource
 */
void FilterModule::receive(Packet* p)
{
	vector<PacketProcessor *>::iterator it;

	// set to true so packet is always pushed to receiver
	// even if the processors-iterator below is NULL
	bool keepPacket=true;

	DPRINTFL(MSG_VDEBUG, "got packet");

	// run packet through all packetProcessors
	for (it = processors.begin(); 
	     it != processors.end() && keepPacket; ++it) 
	{
		keepPacket = (*it)->processPacket(p);
	}

	// check if we passed all filters
	if (keepPacket) {
		// push packet to the receiver
		DPRINTF("pushing packet %d", p);

		dest->receive(p);
		return;
	}
	
	// immediately drop the packet
	DPRINTF("releasing packet");
	p->removeReference();
}

//FIXME: this function is unneccessary, only here to help restructuring
bool FilterModule::hasReceiver()
{ 
	return isConnected(); 
}

void FilterModule::addProcessor(PacketProcessor *p)
{
	processors.push_back(p);
}

std::vector<PacketProcessor *> FilterModule::getProcessors()
{
	return processors;
}





