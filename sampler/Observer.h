/*
 PSAMP Reference Implementation
 Observer.cpp
 Declarations for observing process
 Author: Michael Drueing <michael@drueing.de>

 changed by: Ronny T. Lampert
 changed by: Lothar Braun
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
#include <string>
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
	Observer(const std::string& interface) : thread(Observer::observerThread), allDevices(NULL),
		captureDevice(NULL), capturelen(CAPTURE_LENGTH), pcap_timeout(PCAP_TIMEOUT), 
		pcap_promisc(1), ready(false), filter_exp(0), exitFlag(false)
        {
		captureInterface = (char*)malloc(interface.size() + 1);
		strcpy(captureInterface, interface.c_str());
	};

	~Observer();

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
        int getPcapStats(struct pcap_stat *out)
        {
                return(pcap_stats(captureDevice, out));
        }


        bool prepare(const std::string& filter);
	static void doLogging(void *arg);


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

	// save the given filter expression
	char* filter_exp;

        static void *observerThread(void *);
public:
        bool exitFlag;
        // interface we capture traffic on - string
	char *captureInterface;

        // vector of Queues that will get the packets we pass out
	std::vector<ConcurrentQueue<Packet*> *> receivers;
};

#endif
