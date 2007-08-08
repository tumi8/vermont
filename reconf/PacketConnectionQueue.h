//
// C++ Interface: PacketConnectionQueue
//
// Description: 
//
//
// Author: Peter Baumann <siprbaum@users.berlios.de>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef PACKETCONNECTIONQUEUE_H
#define PACKETCONNECTIONQUEUE_H

/**
	@author Peter Baumann <siprbaum@users.berlios.de>
*/

#include "reconf/PacketDestination.h"
#include "reconf/PacketSource.h"

#include "common/ConcurrentQueue.h"

class PacketConnectionQueue
	: public PacketSource, public PacketDestination
//	 , public ConnectionQueue
{
public:
	PacketConnectionQueue();
	virtual ~PacketConnectionQueue();

	virtual void receive(Packet*);
	
private:
	void process();
	
	Packet* packet;

	ConcurrentQueue<Packet*> queue;
};

#endif
