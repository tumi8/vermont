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
	Destination* d;
	
	/* FIXME: synchronisation handling if this is called in an own thread;
	 *	  especially if one is allowed to disconnect the from the queue
	 */
	while ((d = dest) != NULL) {
		queue.pop(&packet);
		/* this should work if dest is disconnected between the while() and the actual call
		 * of receive() IF dest isn't freed already
		 */
		d->receive(packet);
	}
}
