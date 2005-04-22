/*
 this is vermont.
 released under GPL v2

 (C) by Ronny T. Lampert

 pure abstract class for the sink

 */
#ifndef SINK_H
#define SINK_H

#include "Globals.h"
#include "PacketReceiver.h"

class Sink : public PacketReceiver {

public:
        virtual bool runSink()=0;
        virtual bool terminateSink()=0;

};

#endif
