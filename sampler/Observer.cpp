/*
 PSAMP Reference Implementation
 Observer.cpp
 Implementation of the packet capturing thread
 Author: Michael Drueing <michael@drueing.de>

 changed by: Ronny T. Lampert

 */

#include <pcap.h>
#include <iostream>
#include <unistd.h>

#include "msg.h"

#include "Observer.h"
#include "Thread.h"

using namespace std;

/*
 This is the main observer loop. It graps packets from libpcap and
 dispatches them to the registered receivers.
 */
void *Observer::observerThread(void *arg)
{
        /* first we need to get the instance back from the void *arg */
        Observer *obs = (Observer *)arg;
        Packet *p;
        int numReceivers = obs->receivers.size();
        const unsigned char *rawPacketData;
        void *myPacketData;
        struct pcap_pkthdr packetHeader;

        // start capturing packets
        msg(MSG_INFO, "Observer: now running capturing thread for device %s", obs->captureInterface);

        while(!obs->exitFlag) {
                /*
                 get next packet (no zero-copy possible *sigh*)
                 NOTICE: potential bottleneck, if pcap_next() is calling gettimeofday() at a high rate;
                 there is partially caching function described in an Sun or IBM (Developerworks) article
                 that can act as a via LD_PRELOAD used overlay function.
                 unfortunately I don't have an URL ready -Freek
                 */
                rawPacketData = pcap_next(obs->captureDevice, &packetHeader);
                if(!rawPacketData)
                        /* no packet data was available */
                        continue;

                if(!(myPacketData=malloc(packetHeader.caplen))) {
                        /*
                         FIXME!
                         ALARM - no more memory available
                         1) Start throwing away packets !
                         2) Notify user !
                         3) Try to resolve (?)
                         3.1) forcibly flush exporter stream (to free up packets)?
                         3.2) flush filter?
                         3.3) sleep?
                         */
                        msg(MSG_FATAL, "Observer: no more mem for malloc() - may start throwing away packets");
                        continue;
                }

                memcpy(myPacketData, rawPacketData, packetHeader.caplen);

                p = new Packet(myPacketData, packetHeader.caplen, numReceivers);
                p->timestamp = packetHeader.ts;
                /*
                DPRINTF("Observer: received packet at %d.%04d, len=%d\n",
                    p->timestamp.tv_sec,
                    p->timestamp.tv_usec / 1000,
                    packetHeader.caplen
                   );
                 */

                /* broadcast packet to all receivers */
                for(vector<ConcurrentQueue<Packet> *>::iterator it = obs->receivers.begin();
                    it != obs->receivers.end(); ++it) {
                        (*it)->push(p);
                }
        }

        pthread_exit((void *)1);
}

