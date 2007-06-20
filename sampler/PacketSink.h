/*
 * PSAMP Reference Implementation
 *
 * PacketSink.h
 *
 * Implementation of a dummy packet sink
 *
 * Author: Michael Drueing <michael@drueing.de>
 *
 */

#ifndef PACKET_SINK_H
#define PACKET_SINK_H

#include "msg.h"

#include "Thread.h"
#include "Sink.h"

class PacketSink : public Sink
{
public:
        PacketSink() : thread(PacketSink::packetSinkProcess), exitFlag(false)
        {
#if defined(DEBUG)
	    queue->debugSetOwner("PacketSink");
#endif
        };

        ~PacketSink()
        {
                msg(MSG_DEBUG, "destructor called");
                terminateSink();
        };

        bool runSink()
        {
                msg(MSG_DEBUG, "now starting PacketSink thread");
                return(thread.run(this));
        };

        /* this really could be more elaborate */
        bool terminateSink()
        {
                exitFlag = true;

                msg(MSG_DEBUG, "waiting for exporter thread");
                thread.join();
                msg(MSG_DEBUG, "exporter thread joined");

                return true;
        };

protected:
        Thread thread;
        static void *packetSinkProcess(void *);

public:
        bool exitFlag;
};

#endif
