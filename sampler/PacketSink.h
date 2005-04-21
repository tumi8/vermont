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

#include "Globals.h"
#include "Thread.h"
#include "PacketReceiver.h"

class PacketSink : public PacketReceiver
{
public:
        PacketSink() : thread(PacketSink::packetSinkProcess), exitFlag(false)
        {
        };

        ~PacketSink()
        {
        };

        bool runSink()
        {
                msg(MSG_DEBUG, "Sink: now starting PacketSink thread");
                return(thread.run(this));
        };

        void terminateSink()
        {
                exitFlag = true;
        };

protected:
        Thread thread;
        static void *packetSinkProcess(void *);

public:
        bool exitFlag;
};

#endif
