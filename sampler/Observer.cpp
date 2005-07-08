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
	Observer *obs=(Observer *)arg;
	Packet *p;

        const unsigned char *pcapData;
	void *packetData;
	struct pcap_pkthdr packetHeader;

	int numReceivers=obs->receivers.size();

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
		pcapData=pcap_next(obs->captureDevice, &packetHeader);
		if(!pcapData)
			/* no packet data was available */
			continue;

		if(!(packetData=malloc(packetHeader.caplen))) {
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

		memcpy(packetData, pcapData, packetHeader.caplen);

		/*
		 the reason we supply numReceivers to the packet is, that all receivers have to call
		 packet->release() and only if the count is 0, the packet will be really deleted
		 We need reference-counting because we only push pointers around and do not copy, so the
		 data has to stay valid.
		 */
		p=new Packet(packetData, packetHeader.caplen, numReceivers);
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


void Observer::doLogging(void *arg)
{
	Observer *obs=(Observer *)arg;
	struct pcap_stat stats;

	/*
	 pcap_stats() will set the stats to -1 if something goes wrong
	 so it is okay if we dont check the return code
	 */
	obs->getPcapStats(&stats);
	msg_stat("Observer: %6d recv, %6d drop, %6d ifdrop", stats.ps_recv, stats.ps_drop, stats.ps_ifdrop);
}


//static void plain_c_sucks_because_people_dont_seem_to_have_a_caps_key_and_like_to_type_stupid_underscores_alot(){}
