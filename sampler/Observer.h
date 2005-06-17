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
 some platforms don't support it, though.
 FIXME: HOW LONG? 2000ms is REALLY REALLY LONG!
 On a busy network we may want to have it shorter.
 Maybe this should be runtime-configurable.
 */
#define PCAP_TIMEOUT 100

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
	Observer(char *interface) : thread(Observer::observerThread), allDevices(NULL),
		captureDevice(NULL), capturelen(CAPTURE_LENGTH), pcap_timeout(PCAP_TIMEOUT), 
		pcap_promisc(1), ready(false), exitFlag(false)
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
        bool setCaptureLen(int x)
        {
                /* we cant change pcap caplen if alredy pcap_open() called */
                if(ready) {
                        msg(MSG_ERROR, "Observer: changing capture len on-the-fly is not supported by pcap");
                        return false;
                }

                if(x > CAPTURE_PHYSICAL_MAX) {
                        DPRINTF("Capture length %d exceeds physical MTU %d (with header)\n", x, CAPTURE_PHYSICAL_MAX);
                        return false;
                }
                capturelen=x;
                return true;
        }

        int getCaptureLen()
        {
                return capturelen;
        }


        bool setPacketTimeout(int ms)
        {
                if(ready) {
                        msg(MSG_ERROR, "Observer: changing read timeout on-the-fly is not supported by pcap");
                        return false;
                }
                pcap_timeout=ms;
                return true;
        }


        int getPacketTimeout()
        {
                return pcap_timeout;
        }


        /*
         get some capturing statistics
         struct pcap_stat is defined in pcap.h and has at least 3 u_int variables:
         ps_recv, ps_drop, ps_ifdrop

         should return: -1 on failure, 0 on OK
         */
        int pcapStats(struct pcap_stat *out)
        {
                return(pcap_stats(captureDevice, out));
        }


protected:
        Thread thread;

        // pointer to list of pcap-devices
        pcap_if_t *allDevices;

        // pcap descriptor of device
	pcap_t *captureDevice;

        // IPv4 netmask + network bitmasks the interface is on
	uint32_t netmask, network;

        // holding the pcap filter program
	struct bpf_program pcap_filter;

        // pcap reports error nicely, this is the used buffer
	char errorBuffer[PCAP_ERRBUF_SIZE];

        // also called snaplen; only sniff this much bytes from each packet
	int capturelen;

        // wait this much ms until pcap_read() returns and get ALL packets received
	int pcap_timeout;

        // capture packets in promiscous mode or not
	int pcap_promisc;

        // set to true if prepare() was successful
	bool ready;

        static void *observerThread(void *);

public:
        bool exitFlag;
        // interface we capture traffic on - string
	char *captureInterface;

        // vector of Queues that will get the packets we pass out
	std::vector<ConcurrentQueue<Packet> *> receivers;
};

#endif
