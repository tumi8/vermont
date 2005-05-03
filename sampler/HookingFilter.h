/*
 Hooking Filter
 (c) by Ronny T. Lampert

 This filter calls a given function pointer for every packet it receives.
 Because we assume standard C code that doesn't know about class Packet
 we have to marshall the relevant pointers into a struct first.

 */

#ifndef HOOKING_FILTER_H
#define HOOKING_FILTER_H

#include "PacketProcessor.h"

class HookingFilter : public PacketProcessor {

public:

	HookingFilter(void (*hook)(void *)) {
                f=hook;
	}

	~HookingFilter() {

	}

	virtual bool processPacket(const Packet *p);

protected:

	/*
	 this is called "f" because our mentor created that name accidently
	 so it was meant to stay, because everyone knew what it represents
         */
	void (*f)(void *);

};




#endif
