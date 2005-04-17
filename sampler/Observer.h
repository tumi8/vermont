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
#define PCAP_TIMEOUT 2000
/*
 maximum physical packet length
 you may want to adjust this on a special jumbo-framed GBit network
 */
#define CAPTURE_PHYSICAL_MAX 1526
#include <vector>
#include <pcap.h>

#include "Globals.h"
#include "Thread.h"
#include "ConcurrentQueue.h"
#include "Packet.h"
#include "PacketReceiver.h"

#include "msg.h"

class Observer {
public:
        // ObserverThread constructor
	Observer(char *interface) : thread(Observer::observerThread), ready(false), exitFlag(false),
		capturelen(CAPTURE_LENGTH), pcap_timeout(PCAP_TIMEOUT)
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

		pcap_freealldevs(allDevices);
        };

	/*
	 call after an Observer has been created
	 some error checking on pcap here, because it can't be done in the constructor
         pcap_open() can only be called in the thread, because then we'll start getting packets
	 */
	bool open()
	{
		// query all available capture devices
		msg(MSG_INFO, "Observer: Finding devices");
                pcap_findalldevs(&allDevices, errorBuffer);
                if(!allDevices) {
			msg(MSG_FATAL, "Observer: error getting list of interfaces: %s", errorBuffer);
			return false;
		}

		for(pcap_if_t *dev = allDevices; dev != NULL; dev=dev->next) {
			msg(MSG_INFO, "PCAP: name=%s, desc=%s", dev->name, dev->description);
                }
		ready=true;
		return true;
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

        bool Observer::setCaptureLen(int x)
        {
                if(x > CAPTURE_PHYSICAL_MAX) {
                        LOG("Capture length %d exceeds physical MTU %d (with header)\n", x, CAPTURE_PHYSICAL_MAX);
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
        char errorBuffer[PCAP_ERRBUF_SIZE];
	int capturelen;
        int pcap_timeout;
        bool ready;

        static void *observerThread(void *);

public:
        bool exitFlag;
        char *captureInterface;
        std::vector<ConcurrentQueue<Packet> *> receivers;
};

#endif
