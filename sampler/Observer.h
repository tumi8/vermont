/*
 PSAMP Reference Implementation
 Observer.cpp
 Declarations for observing process
 Author: Michael Drueing <michael@drueing.de>

 changed by: Ronny T. Lampert
 */

#ifndef OBSERVER_H
#define OBSERVER_H

/* default pcap packet capture length */
#define CAPTURE_LENGTH 128
/*
 the to_ms arg to pcap_open_live() - wait this long until returning from pcap_next()
 some platforms don't support it
 FIXME: HOW LONG? 2000ms is REALLY REALLY LONG!
 */
#define PCAP_TIMEOUT 1000
/*
 maximum physical packet length
 you may want to adjust this on a special jumbo-framed GBit network
 */
#define CAPTURE_PHYSICAL_MAX 1526
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pcap.h>

#include "Thread.h"
#include "ConcurrentQueue.h"
#include "Packet.h"
#include "PacketReceiver.h"

#include "msg.h"

class Observer {
public:
        // ObserverThread constructor
	Observer(char *interface) : thread(Observer::observerThread),
		ready(false), exitFlag(false),
		capturelen(CAPTURE_LENGTH), pcap_timeout(PCAP_TIMEOUT), pcap_promisc(1),
		allDevices(NULL), captureDevice(NULL)
        {
		captureInterface=interface;
	};

	~Observer()
        {
		terminateCapture();
                /* be sure the thread is ending */
		msg(MSG_DEBUG, "Observer: joining the ObserverThread, may take a while");
		thread.join();

		msg(MSG_DEBUG, "Observer: freeing pcap/devices");
                if(captureDevice) {
                        pcap_close(captureDevice);
                }

                /* no pcap_freecode here, is already done after attaching the filter */

                if(allDevices) {
                        pcap_freealldevs(allDevices);
                }

		msg(MSG_DEBUG, "Observer: successful shutdown");
        };

	/*
	 call after an Observer has been created
         error checking on pcap here, because it can't be done in the constructor
         and it may be too late, if done in the thread
         */
	bool prepare(char *filter_exp)
	{
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

                captureDevice=pcap_open_live(captureInterface, capturelen, pcap_promisc, pcap_timeout, errorBuffer);
                // check for errors
                if(!captureDevice) {
                        msg(MSG_FATAL, "Observer: Error initializing pcap interface: %s", errorBuffer);
                        goto out1;
                }

                /* we need the netmask for the pcap_compile */
                if(pcap_lookupnet(captureInterface, &network, &netmask, errorBuffer) == -1) {
                        msg(MSG_FATAL, "Observer: unable to determine netmask/network: %s", errorBuffer);
                        goto out2;
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
	 call to get the main capture thread running
         open() has to be called before
	 */
	bool startCapture()
        {
		if(ready) {
			msg(MSG_DEBUG, "Observer: now starting capturing thread");
			return(thread.run(this));
		}
		msg(MSG_ERROR, "Observer: trying to start an un-ready observer");
		return false;
        };

        void terminateCapture()
        {
                exitFlag = true;
        };

        void addReceiver(PacketReceiver *recv)
        {
                receivers.push_back(recv->getQueue());
        };

        /* you cannot change the caplen of an already running observer */
        bool Observer::setCaptureLen(int x)
        {
                if(x > CAPTURE_PHYSICAL_MAX) {
                        DPRINTF("Capture length %d exceeds physical MTU %d (with header)\n", x, CAPTURE_PHYSICAL_MAX);
                        return false;
                }
                capturelen=x;
                return true;
        }

        int Observer::getCaptureLen()
        {
                return capturelen;
        }

protected:
        Thread thread;

        // pointer to list of pcap-devices
        pcap_if_t *allDevices;
        pcap_t *captureDevice;
        uint32_t netmask, network;
        struct bpf_program pcap_filter;

        char errorBuffer[PCAP_ERRBUF_SIZE];
	int capturelen;
        int pcap_timeout;
        int pcap_promisc;
        bool ready;

        static void *observerThread(void *);

public:
        bool exitFlag;
        char *captureInterface;
        std::vector<ConcurrentQueue<Packet> *> receivers;
};

#endif
