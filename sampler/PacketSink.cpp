/*
 * PSAMP Reference Implementation
 *
 * PacketSink.cpp
 *
 * Implementation of a simple packet sink
 *
 * Author: Michael Drueing <michael@drueing.de>
 *
 */
#include <iostream>

#include "common/msg.h"
#include "PacketSink.h"

using namespace std;

// tobi_optimize: this is a useless thread, it just exists to clean up old packets
// this can be done by the filter too
void *PacketSink::packetSinkProcess(void *arg)
{
	PacketSink *sink = (PacketSink *)arg;
	ConcurrentQueue<Packet*> *queue = sink->getQueue();
	Packet *p;

	msg(MSG_INFO, "now running PacketSink thread");
	while(!sink->exitFlag) {
		if (!queue->pop(&p)) break;
		DPRINTF("dropping packet");
		p->removeReference();
	}
	return 0;
}

