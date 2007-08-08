//
// C++ Interface: peterpacketsource
//
// Description: 
//
//
// Author: Peter Baumann <siprbaum@users.berlios.de>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef PACKETSOURCE_H
#define PACKETSOURCE_H

/**
	@author Peter Baumann <siprbaum@users.berlios.de>
*/


#include "reconf/Source.h"
#include "reconf/PacketDestination.h"

#include "sampler/Packet.h"

class PacketSource: public Source
{
public:
	PacketSource() : dest(NULL) { }
	
	virtual void connectTo(Destination* dest);
	virtual bool isConnected() const;
	virtual void disconnect();

protected:
	PacketDestination* dest;
};

#endif
