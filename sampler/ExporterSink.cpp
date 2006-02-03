/*
 * PSAMP Reference Implementation
 *
 * ExporterSink.cpp
 *
 * Implementation of an IPFIX exporter packet sink
 * using Jan Petranek's ipfixlolib
 *
 * Changed by Gerhard Muenz
 *   return value of addPacket evaluated
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

void *ExporterSink::exporterSinkProcess(void *arg)
{
	ExporterSink *sink = (ExporterSink *)arg;
	ConcurrentQueue<Packet> *queue = sink->getQueue();
	Packet *p;
	bool result;
	// our deadline
	struct timeval deadline;
	int pckCount;

	msg(MSG_INFO, "Sink: now running ExporterSink thread");

	while(!sink->exitFlag) {
		sink->startNewPacketStream();

		// let's get the first packet
		do
		{
		    p = queue->pop();
		}
		while(sink->addPacket(p) == false);

		pckCount = 1;

		// now calculate the deadline by which the packet has to leave the exporter
		gettimeofday(&deadline, 0);
		deadline.tv_usec += sink->exportTimeout * 1000L;
		if(deadline.tv_usec > 1000000L) {
			deadline.tv_sec += (deadline.tv_usec / 1000000L);
			deadline.tv_usec %= 1000000L;
		}

		while(pckCount < sink->ipfix_maxpackets) {
			// Try to get next packet from queue before our deadline
			result = queue->popAbs(deadline, &p);

			// check for timeout and break loop if neccessary
			if(!result)
				break;

			// no timeout received, continue waiting, but

			// count only if packet was added
			if(sink->addPacket(p) == true)
			    pckCount++;
		}
		sink->flushPacketStream();
	}
	return 0;
}


bool ExporterSink::addCollector(char *address, unsigned short port, const char *protocol)
{
	ipfix_transport_protocol proto;

	if(strcasecmp(protocol, "TCP") == 0) {
		proto = TCP;
	} else if(strcasecmp(protocol, "UDP") == 0) {
		proto = UDP;
	} else {
		msg(MSG_ERROR, "ExporterSink: invalid protocol %s for %s",
		    protocol, address);
		return false;
	}

	DPRINTF("Adding %s://%s:%d\n", protocol, address, port);
	return(ipfix_add_collector(exporter, address, port, proto) == 0);
}
