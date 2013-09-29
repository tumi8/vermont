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

#include "common/msg.h"
#include "common/Thread.h"
#include "common/ConcurrentQueue.h"

#include "core/InstanceManager.h"
#include "core/Source.h"
#include "core/Module.h"

#include <vector>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pcap.h>

class Observer : public Module, public Source<Packet*>, public Destination<NullEmitable*>
{
public:
	Observer(const std::string& interface, bool offline, uint64_t maxpackets);
	~Observer();

	virtual void performStart();
	virtual void performShutdown();
	bool setCaptureLen(int x);
	void setOfflineAutoExit(bool autoexit);
	int getCaptureLen();
	bool setPacketTimeout(int ms);
	int getPacketTimeout();
	void replaceOfflineTimestamps();
	void setOfflineSpeed(float m);
	int getPcapStats(struct pcap_stat *out);
	bool prepare(const std::string& filter);
	static void doLogging(void *arg);
	virtual std::string getStatisticsXML(double interval);


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
	uint32_t capturelen;

	// wait this much ms until pcap_read() returns and get ALL packets received
	int pcap_timeout;

	// capture packets in promiscous mode or not
	int pcap_promisc;

	// maximum number of packets to capture, then stop processing
	// 0 == do not stop
	uint64_t maxPackets;

	// set to true if prepare() was successful
	bool ready;

	// save the given filter expression
	char* filter_exp;

	// manages instances of Packets
	static InstanceManager<Packet> packetManager;

	uint32_t observationDomainID;

	// number of received bytes (used for statistics)
	// attention: value may sometimes be incorrect caused by multithreading issues
	volatile uint64_t receivedBytes;
	volatile uint64_t lastReceivedBytes;

	// number of processed packets (used for statistics)
	// attention: value may sometimes be incorrect caused by multithreading issues
	volatile uint64_t processedPackets;
	volatile uint64_t lastProcessedPackets;

	// interface we capture traffic on - string
	char *captureInterface;

	// pcap file we read traffic from - string
	char *fileName;

	// offline mode parameters
	bool readFromFile;
	bool replaceTimestampsFromFile;
	uint16_t stretchTimeInt; // 1 means no timing change, 0 means that stretchTimes (float) is used
	float stretchTime;
	bool autoExit;

	bool slowMessageShown;	// true if message was shown that vermont is too slow to read file in time

	uint32_t statTotalLostPackets;
	uint32_t statTotalRecvPackets;

	static void *observerThread(void *);

	int dataLinkType; // contains the datalink type of the capturing device
};

#endif
