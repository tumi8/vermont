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
	
	virtual void receive(Emitable*) = 0;
};

template<class T>
class Destination : public virtual BaseDestination
{
public:
	typedef T dst_value_type;
	
	virtual void receive(Emitable* e)
	{
		T p = dynamic_cast<T>(e);
		if (p == NULL)
			THROWEXCEPTION("module does not support receiving this type of modules");
	}

	virtual void receive(T e) = 0;

private:
	inline void recvPacket(T p) {
		printf("recv called\n");
	}
};

template<>
class Destination<NullEmitable*> : public BaseDestination
{
public:
	typedef NullEmitable* dst_value_type;

	virtual void receive(Emitable* e)
	{
		THROWEXCEPTION("this module is no destination!");
	}
};


#endif

