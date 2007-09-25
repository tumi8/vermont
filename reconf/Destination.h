#ifndef PETERSINK_H
#define PETERSINK_H

/**
	@author Peter Baumann <siprbaum@users.berlios.de>
*/

#include "reconf/Emitable.h"
#include "common/msg.h"

#include <cstdio>
#include <stdexcept>

class BaseDestination {
public:
	virtual ~BaseDestination();
};

template<class T>
class Destination : public virtual BaseDestination
{
public:
	virtual void receive(T e)
	{
		THROWEXCEPTION("module does not support reception of elements");
	}


private:
	inline void recvPacket(T p) {
		printf("recv called\n");
	}
};

#endif

