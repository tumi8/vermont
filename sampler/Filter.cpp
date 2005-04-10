/*
 * PSAMP Reference Implementation
 *
 * Filter.cpp
 *
 * Main filtering loop. Accepts a packet and applies filters on it
 *
 * Author: Michael Drueing <michael@drueing.de>
 *
 */
 
#include "Filter.h"

using namespace std;

Filter *Filter::instance = 0;

void *Filter::filterProcess(void *arg)
{
  Filter *filter = Filter::instance;
  Packet *p;
  bool keepPacket;
  vector<PacketProcessor *>::iterator it;

  while (!filter->exitFlag)
  {
    // get a packet...
    p = filter->getQueue()->pop();
    filter->pktIn++;
    
    // run packet through all packetProcessors
    for (it = filter->processors.begin(); it != filter->processors.end(); ++it)
    {
      keepPacket = (*it)->processPacket(p);
      if (!keepPacket)
        break;
    }
    
    //check if we passed all filters
    if (keepPacket)
    {
      // get the packet to the receiver
      filter->receiver->push(p);
      filter->pktOut++;
    }
    else
    {
      // immediately drop the packet
      p->release();
    }
  }
};

