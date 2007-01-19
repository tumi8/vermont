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

#include "msg.h"
#include "PacketSink.h"

using namespace std;

void *PacketSink::packetSinkProcess(void *arg)
{
        PacketSink *sink = (PacketSink *)arg;
        ConcurrentQueue<Packet> *queue = sink->getQueue();
        Packet *p;

        msg(MSG_INFO, "Sink: now running PacketSink thread");
        while(!sink->exitFlag) {
                p = queue->pop();
                p->release();
                DPRINTF("SINK: free packet");
        }
	return 0;
}

