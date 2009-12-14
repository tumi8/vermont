/*
 * Vermont Packet Filter
 * Copyright (C) 2009  Vermont Project
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
 *
 */

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
