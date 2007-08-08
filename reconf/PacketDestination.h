//
// C++ Interface: PacketDestination
//
// Description: 
//
//
// Author: Peter Baumann <siprbaum@users.berlios.de>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef PACKETDESTINATION_H
#define PACKETDESTINATION_H

/**
	@author Peter Baumann <siprbaum@users.berlios.de>
*/


#include "reconf/Destination.h"
#include "sampler/Packet.h"

class PacketDestination: public Destination 
{
public:
	virtual ~PacketDestination() { };

	virtual void receive(Emitable *);
	virtual void receive(Packet *) = 0;
};

#endif
