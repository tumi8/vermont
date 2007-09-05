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


/*
 the to_ms arg to pcap_open_live() - wait this long until returning from pcap_next()
 some platforms don't support it, though.
 FIXME: HOW LONG? 2000ms is REALLY REALLY LONG!
 On a busy network we may want to have it shorter.
 Maybe this should be runtime-configurable.
 */
#define PCAP_TIMEOUT 100


#include "Packet.h"
#include "PacketReceiver.h"

#include "common/msg.h"
#include "common/Thread.h"
#include "common/ConcurrentQueue.h"
#include "common/InstanceManager.h"
#include "common/StatisticsManager.h"

#include <vector>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pcap.h>

class Observer : public StatisticsModule {

public:
	Observer(const std::string& interface, InstanceManager<Packet>* manager);
	~Observer();

	void startCapture();
	void terminateCapture();
	void addReceiver(PacketReceiver *recv);
	bool setCaptureLen(int x);
	int getCaptureLen();
	bool setPacketTimeout(int ms);
	int getPacketTimeout();
	int getPcapStats(struct pcap_stat *out);
	bool prepare(const std::string& filter);
	static void doLogging(void *arg);
	virtual std::string getStatistics();


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

	// manages instances of Packets
	InstanceManager<Packet>* packetManager;

	// number of received bytes (used for statistics)
	// attention: value may sometimes be incorrect caused by multithreading issues
	volatile uint64_t receivedBytes;
	volatile uint64_t lastReceivedBytes;

	// number of processed packets (used for statistics)
	// attention: value may sometimes be incorrect caused by multithreading issues
	volatile uint64_t processedPackets;
	volatile uint64_t lastProcessedPackets;

	static void *observerThread(void *);

public:
	// is true, when application is to be shut down
	bool exitFlag;

	// interface we capture traffic on - string
	char *captureInterface;

	// vector of Queues that will get the packets we pass out
	std::vector<ConcurrentQueue<Packet*> *> receivers;
};

class ObserverStatModule : public StatisticsModule
{
	private:
		Observer* observer;
		uint64_t lastDPackets;
		uint64_t lastRPackets;

	public:
		ObserverStatModule(Observer* obs);
		virtual std::string getStatistics();
};

#endif
