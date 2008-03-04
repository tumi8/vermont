#ifndef PETERSINK_H
#define PETERSINK_H

/**
	@author Peter Baumann <siprbaum@users.berlios.de>
*/

#include "reconf/Emitable.h"
#include "common/msg.h"

#include <cstdio>
#include <stdexcept>

template<class T>
class Destination
{
public:
	typedef T dst_value_type;
	
	virtual ~Destination() { };
	
	virtual void receive(T e) = 0;
};

template<>
class Destination<NullEmitable*> 
{
public:
	typedef NullEmitable* dst_value_type;

	virtual ~Destination() { };
	
	virtual void receive(Emitable* e)
	{
		THROWEXCEPTION("this module is no destination!");
	}
};


#endif

