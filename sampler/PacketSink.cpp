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
#include "PacketSink.h"

using namespace std;

void *PacketSink::packetSinkProcess(void *arg)
{
  PacketSink *sink = (PacketSink *)arg;
  ConcurrentQueue<Packet> *queue = sink->getQueue();
  Packet *p;
  
  cerr << "PacketSink started" << endl;
  while (!sink->exitFlag)
  {
    p = queue->pop();
    p->release();
    //cerr << "*";
  }
}

