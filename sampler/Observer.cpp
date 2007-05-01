/*
 PSAMP Reference Implementation
 Observer.cpp
 Implementation of the packet capturing thread
 Author: Michael Drueing <michael@drueing.de>

 changed by: Ronny T. Lampert
             Gerhard Münz
 */

#include <pcap.h>
#include <iostream>
#include <unistd.h>

#include "msg.h"

#include "Observer.h"
#include "Thread.h"

using namespace std;


Observer::~Observer()
{
    msg(MSG_DEBUG, "Observer: destructor called");
    terminateCapture();

    /* collect and output statistics */
    pcap_stat pstats;
    if (captureDevice && pcap_stats(captureDevice, &pstats)==0) {
        msg(MSG_DIALOG, "PCAP statistics:");
        msg(MSG_DIALOG, "Number of packets received: %u", pstats.ps_recv);
        msg(MSG_DIALOG, "Number of packets dropped: %u", pstats.ps_drop);
    }

    /* be sure the thread is ending */
    msg(MSG_DEBUG, "Observer: joining the ObserverThread, may take a while (until next pcap data is received)");
    thread.join();
    msg(MSG_DEBUG, "Observer: ObserverThread joined");

    msg(MSG_DEBUG, "Observer: freeing pcap/devices");
    if(captureDevice) {
        pcap_close(captureDevice);
    }

    /* no pcap_freecode here, is already done after attaching the filter */

    if(allDevices) {
        pcap_freealldevs(allDevices);
    }

    free(captureInterface);
    delete[] filter_exp;
    msg(MSG_DEBUG, "Observer: successful shutdown");
}
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
		p=new Packet(packetData, packetHeader.caplen, packetHeader.ts, numReceivers);

		DPRINTF("Observer: received packet at %u.%04u, len=%d\n",
		(unsigned)p->timestamp.tv_sec,
		(unsigned)p->timestamp.tv_usec / 1000,
		packetHeader.caplen
		);

		/* broadcast packet to all receivers */
		if (!obs->exitFlag) {
		    for(vector<ConcurrentQueue<Packet*> *>::iterator it = obs->receivers.begin();
			    it != obs->receivers.end(); ++it) {
			if ((*it)->getCount() > 100000) {
			    msg(MSG_FATAL, "Observer: Observer drain clogged, waiting for plumber");
			    while ((*it)->getCount() > 10000) sleep(1);
			    msg(MSG_FATAL, "Observer: drain not clogged any more, resuming operation");
			}
			(*it)->push(p);
		    }
		}
	}

	msg(MSG_DEBUG, "Observer: exiting observer thread");
	pthread_exit((void *)1);
}


/*
 call after an Observer has been created
 error checking on pcap here, because it can't be done in the constructor
 and it may be too late, if done in the thread
 */
bool Observer::prepare(const std::string& filter)
{
	int dataLink = 0;

	// we need to store the filter expression, because pcap needs
	// a char* and doesn't accept a const char* ... nasty pcap-devs!!!
	if (!filter.empty()) {
		filter_exp = new char[filter.size() + 1];
		strcpy(filter_exp, filter.c_str());
	}
	struct in_addr i_netmask, i_network;

	// query all available capture devices
	msg(MSG_INFO, "Observer: Finding devices");
	if(pcap_findalldevs(&allDevices, errorBuffer) == -1) {
		msg(MSG_FATAL, "Observer: error getting list of interfaces: %s", errorBuffer);
		goto out;
	}

	for(pcap_if_t *dev = allDevices; dev != NULL; dev=dev->next) {
		msg(MSG_DEBUG, "PCAP: name=%s, desc=%s", dev->name, dev->description);
	}

	msg(MSG_INFO,
	    "Observer: pcap opening interface=%s, promisc=%d, snaplen=%d, timeout=%d",
	    captureInterface, pcap_promisc, capturelen, pcap_timeout
	   );
	captureDevice=pcap_open_live(captureInterface, capturelen, pcap_promisc, pcap_timeout, errorBuffer);
	// check for errors
	if(!captureDevice) {
		msg(MSG_FATAL, "Observer: Error initializing pcap interface: %s", errorBuffer);
		goto out1;
	}

	dataLink = pcap_datalink(captureDevice);
	// IP_HEADER_OFFSET is set by the configure script
	switch (dataLink) {
	case DLT_EN10MB:
		if (IP_HEADER_OFFSET != 14 && IP_HEADER_OFFSET != 18) {
			msg(MSG_FATAL, "Observer: IP_HEADER_OFFSET on an ethernet device has to be 14 or 18 Bytes. Please adjust that value via configure --with-ipheader-offset");
			goto out2;
		}
		break;
	case DLT_LOOP:
	case DLT_NULL:
		if (IP_HEADER_OFFSET != 4) {
			msg(MSG_FATAL, "Observer: IP_HEADER_OFFSET on BSD loop back device has to be 4 Bytes. Please adjust that value via configure --with-ipheader-offset");
			goto out2;
		}
		break;
	default:
		msg(MSG_ERROR, "Observer: You are using an unkown IP_HEADER_OFFSET and data link combination. This can make problems. Please check if you use the correct IP_HEADER_OFFSET for your data link, if you see strange IPFIX/PSAMP packets.");
	}


	/* we need the netmask for the pcap_compile */
	if(pcap_lookupnet(captureInterface, &network, &netmask, errorBuffer) == -1) {
		msg(MSG_ERROR, "Observer: unable to determine netmask/network: %s", errorBuffer);
		network=0;
		netmask=0;
	}
	i_network.s_addr=network;
	i_netmask.s_addr=netmask;
	msg(MSG_DEBUG, "Observer: pcap seems to run on network %s", inet_ntoa(i_network));
	msg(MSG_INFO, "Observer: pcap seems to run on netmask %s", inet_ntoa(i_netmask));

	if(filter_exp) {
		msg(MSG_DEBUG, "Observer: compiling pcap filter code from: %s", filter_exp);
		if(pcap_compile(captureDevice, &pcap_filter, filter_exp, 1, netmask) == -1) {
			msg(MSG_FATAL, "Observer: unable to validate+compile pcap filter");
			goto out2;
		}

		if(pcap_setfilter(captureDevice, &pcap_filter) == -1) {
			msg(MSG_FATAL, "Observer: unable to attach filter to pcap: %s", pcap_geterr(captureDevice));
			goto out3;
		}
		/* you may free an attached code, see man-page */
		pcap_freecode(&pcap_filter);
	} else {
		msg(MSG_DEBUG, "Observer: using no pcap filter");
	}

	ready=true;

	return true;

out3:
	pcap_freecode(&pcap_filter);
out2:
	pcap_close(captureDevice);
	captureDevice=NULL;
out1:
	pcap_freealldevs(allDevices);
	allDevices=NULL;
out:
	return false;
}


/*
 this function is called by the logger timer thread and should dump
 some nice info using msg_stat
 */
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
