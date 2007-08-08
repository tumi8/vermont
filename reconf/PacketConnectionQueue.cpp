//
// C++ Implementation: PacketConnectionQueue
//
// Description: 
//
//
// Author: Peter Baumann <siprbaum@users.berlios.de>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "PacketConnectionQueue.h"

PacketConnectionQueue::PacketConnectionQueue()
	: queue()
{
}

PacketConnectionQueue::~PacketConnectionQueue()
{
}

void PacketConnectionQueue::receive(Packet* packet)
{
	printf("receive(Packet*)\n");
	queue.push(packet);
	
	// FIXME: this should be in an own thread
	process();
	
}

void PacketConnectionQueue::process()
{
	Packet* packet = NULL;
	
	while (dest) {
		queue.pop(&packet);
		dest->receive(packet);
	}
}
