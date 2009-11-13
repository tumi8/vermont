#include "modules/packet/filter/FilterModule.h"

#include <sstream>

using namespace std;

FilterModule::FilterModule()
{
}

FilterModule::~FilterModule()
{
	vector<PacketProcessor *>::iterator it;

	// delete allpacketProcessors
	for (it = processors.begin(); it != processors.end(); ++it)
		delete *it;
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

	DPRINTFL(MSG_VDEBUG, "FilterModule: got packet");

	// run packet through all packetProcessors
	for (it = processors.begin();
	     it != processors.end() && keepPacket; ++it)
	{
		keepPacket = (*it)->processPacket(p);
	}

	// check if we passed all filters
	if (keepPacket) {
		// push packet to the receiver
		DPRINTF("FilterModule: pushing packet %d", p);
		while (!exitFlag && !send(p));
		return;
	}

	// immediately drop the packet
	DPRINTF("FilterModule: releasing packet");
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

/**
 * statistics function called by StatisticsManager
 */
std::string FilterModule::getStatisticsXML(double interval)
{
	ostringstream oss;
	for (vector<PacketProcessor *>::iterator it = processors.begin(); it != processors.end(); ++it) {
		oss << (*it)->getStatisticsXML(interval);
	}
	return oss.str();
}
