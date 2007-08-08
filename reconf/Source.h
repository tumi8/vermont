#ifndef PETERSOURCE_H
#define PETERSOURCE_H

/**
	@author Peter Baumann <siprbaum@users.berlios.de>
*/

#include "reconf/Destination.h"
#include "reconf/Emitable.h"

#include <cstdio>

class Destination;
class Source;
class Connector;

class Source {
public:
	virtual ~Source() { }
	
	virtual void connectTo(Destination* dest) = 0;
	virtual void disconnect() = 0;

	virtual bool isConnected() const = 0;
};

#endif
