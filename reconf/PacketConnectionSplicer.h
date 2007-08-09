#ifndef PACKETCONNECTIONSPLICER_H
#define PACKETCONNECTIONSPLICER_H

/**
	@author Peter Baumann <siprbaum@users.berlios.de>
*/

#include "reconf/PacketDestination.h"
#include "reconf/PacketSource.h"

#include "common/Mutex.h"

#include <vector>

class PacketConnectionSplicer
	: public PacketSource, public PacketDestination
{
public:
	PacketConnectionSplicer();
	virtual ~PacketConnectionSplicer();

	virtual void connectTo(Destination*);
	virtual bool isConnected() const;
	virtual void receive(Packet*);
	
private:
	void process(Packet *);
	
	Mutex mutex;
	vector<PacketDestination*> destinations;
	vector<PacketDestination*>::size_type size;
};

#endif
