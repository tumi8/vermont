/*
 * PSAMP Reference Implementation
 *
 * ExporterSink.cpp
 *
 * Implementation of an IPFIX exporter packet sink
 * using Jan Petranek's ipfixlolib
 *
 * Author: Michael Drueing <michael@drueing.de>
 *
 */

/*
 TODO
 FIXME: NO TIME CONSTRAINTS HERE
 TODO
 */

#include "msg.h"
#include "ipfixlolib/ipfixlolib.h"

#include "ExporterSink.h"

using namespace std;

void *ExporterSink::exporterSinkProcess(void *arg)
{
        ExporterSink *sink = (ExporterSink *)arg;
        ConcurrentQueue<Packet> *queue = sink->getQueue();
        Packet *p;

        int pckCount = 0;
        // timeout in msec after first packet has been added
        int deadline = 400;

        msg(MSG_INFO, "Sink: now running ExporterSink thread");

        while(!sink->exitFlag) {
                pckCount = 1;

                // first we need to get a packet
                p = queue->pop();
                sink->startNewPacketStream();
                sink->addPacket(p);

                while(pckCount < sink->ipfix_maxpackets) {
                        // TODO: add time constraint here (max. wait time)
                        p = queue->pop();
                        // if (timeout) break;
                        sink->addPacket(p);
                        pckCount++;
                }
                // TODO: add packets here with time constraints
                sink->flushPacketStream();
        }
}
