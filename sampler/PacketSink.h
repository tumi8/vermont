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
        };

        ~PacketSink()
        {
                msg(MSG_DEBUG, "Sink: destructor called");
                terminateSink();
        };

        bool runSink()
        {
                msg(MSG_DEBUG, "Sink: now starting PacketSink thread");
                return(thread.run(this));
        };

        /* this really could be more elaborate */
        bool terminateSink()
        {
                exitFlag = true;

                msg(MSG_DEBUG, "Sink: waiting for exporter thread");
                thread.join();
                msg(MSG_DEBUG, "Sink: exporter thread joined");

                return true;
        };

protected:
        Thread thread;
        static void *packetSinkProcess(void *);

public:
        bool exitFlag;
};

#endif
