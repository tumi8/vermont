#ifndef PETERSINK_H
#define PETERSINK_H

/**
	@author Peter Baumann <siprbaum@users.berlios.de>
*/

#include "reconf/Emitable.h"
#include "reconf/Source.h"

#include <cstdio>
class Source;

class Destination {
public:
	virtual ~Destination() = 0;
	virtual void receive(Emitable *);
};

#endif
