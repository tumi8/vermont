/*
 this is vermont.
 released under GPL v2

 (C) by Ronny T. Lampert

 pure abstract class for the sink

 */
#ifndef SINK_H
#define SINK_H

#include "PacketReceiver.h"

class Sink : public PacketReceiver {

public:
	Sink(string ownerName) : PacketReceiver(ownerName)
	{
	}

	virtual void runSink()=0;
	virtual bool terminateSink()=0;
};

#endif
