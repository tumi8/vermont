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
        virtual void runSink()=0;
        virtual bool terminateSink()=0;

	/*
	 this function is called by the logger timer thread and should dump
	 some nice info using msg_stat
	 */
	static void doLogging(void *arg)
	{
		Sink *snk=(Sink *)arg;

		msg_stat("Sampler: Sink: %6d pushed, %6d popped", snk->queue->pushedCount, snk->queue->poppedCount);
		snk->queue->pushedCount = 0;
		snk->queue->poppedCount = 0;
	}

};

#endif
