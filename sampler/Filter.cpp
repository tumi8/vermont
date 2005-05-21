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
#include "msg.h"
#include "Filter.h"

using namespace std;

Filter *Filter::instance = 0;

void *Filter::filterProcess(void *arg)
{
        Filter *filter = Filter::instance;
        Packet *p;
        bool keepPacket;

        vector<PacketProcessor *>::iterator it;

        /* for dumb compilers, do CSE here to spare some cycles below */
        ConcurrentQueue<Packet> *in_q=filter->getQueue();
        ConcurrentQueue<Packet> *out_q=filter->receiver;

        msg(MSG_INFO, "Filter: now running the filter thread");
        while(!filter->exitFlag) {

                // get a packet...
                p = in_q->pop();
                filter->pktIn++;

                // run packet through all packetProcessors
                for(it = filter->processors.begin(); it != filter->processors.end(); ++it) {
                        keepPacket = (*it)->processPacket(p);
                        if (!keepPacket)
                                break;
                }

                // check if we passed all filters
                if(keepPacket) {
                        // push packet to the receiver
                        out_q->push(p);
                        filter->pktOut++;
                } else {
                        // immediately drop the packet
                        //DPRINTF("Filter: Drop-Freeing packet here");
                        p->release();
                }
        }
};

