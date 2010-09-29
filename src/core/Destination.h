#ifndef DESTINATION_H
#define DESTINATION_H

/**
	@author Peter Baumann <siprbaum@users.berlios.de>
*/

#include "core/Emitable.h"
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

	// See Source.h for comments on the queue running notification
	virtual void notifyQueueRunning() {}
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

	// See Source.h for comments on the Start Signal
	virtual void notifyQueueRunning()
	{
		THROWEXCEPTION("this module is no destination!");
	}
};


#endif

