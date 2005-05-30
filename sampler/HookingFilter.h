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

	HookingFilter(void (*hook)(void*, void *)) {
                f=hook;
	}

	virtual ~HookingFilter() {
	}

        /*
         the hook function may need additional context.
         in our case we need the instance of the aggregator that gets data
         */
        void setContext(void *c) {
		ctx=c;
	}

	void setHook(void (*hook)(void *, void *)) {
                f=hook;
	}

	virtual bool processPacket(const Packet *p);

protected:

	/*
	 this is called "f" because our mentor created that name accidently
	 so it was meant to stay, because everyone knew what it represents
         */
	void (*f)(void *,void *);

	/* we may need a context */
        void *ctx;

};




#endif
