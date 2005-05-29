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

#include <sys/time.h>
#include <time.h>

#include "msg.h"
#include "ipfixlolib/ipfixlolib.h"

#include "ExporterSink.h"

using namespace std;

// maximum time a packet may be in the exporter. in milliseconds.
#define MAX_PACKET_LIFETIME 400

void *ExporterSink::exporterSinkProcess(void *arg)
{
        ExporterSink *sink = (ExporterSink *)arg;
        ConcurrentQueue<Packet> *queue = sink->getQueue();
        Packet *p;
	bool result;
	struct timeval deadline; // our deadline
        int pckCount;

        msg(MSG_INFO, "Sink: now running ExporterSink thread");

        while(!sink->exitFlag) {
                pckCount = 1;

                // first we need to get a packet
                p = queue->pop();
                sink->startNewPacketStream();
                sink->addPacket(p);

		// now calculate the deadline by which the packet has to leave the exporter
		gettimeofday(&deadline, 0);
		deadline.tv_usec += MAX_PACKET_LIFETIME * 1000L;
		if (deadline.tv_usec > 1000000L)
		{
			deadline.tv_sec += (deadline.tv_usec / 1000000L);
			deadline.tv_usec %= 1000000L;
		}

                while(pckCount < sink->ipfix_maxpackets) {
			/* Before:
			p = queue->pop();
			
			sink->addPacket(p);
			pckCount++;
			*/
		
			// Try to get next packet from queue before our deadline
			result = queue->popAbs(deadline, &p);

			// check for timeout and break loop if neccessary
			if (!result)
				break;

			// no timeout received, continue waiting, but 
			
                        sink->addPacket(p);
                        pckCount++;
                }
                // TODO: add packets here with time constraints
		fprintf(stderr, "Flushing packets, reason: %s\n", result ? "MAXPACKETS" : "TIMEOUT");
                sink->flushPacketStream();
        }
}
