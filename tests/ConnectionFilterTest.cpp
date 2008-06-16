#ifdef HAVE_CONNECTION_FILTER

#include "ConnectionFilterTest.h"

#include <sampler/ConnectionFilter.h>
#include <sampler/Packet.h>
#include <common/InstanceManager.h>
#include <common/msg.h>

#include <pcap.h>
#include <iostream>

static void startTests();
static pcap_t *captureDevice;
static char errorBuffer[PCAP_ERRBUF_SIZE]; 


ConnectionFilterTestSuite::ConnectionFilterTestSuite()
        : test_suite("BloomFilterTest")
{
        add(BOOST_TEST_CASE(&startTests));
}

static Packet* getNextPacket(pcap_t* dev)
{
	static InstanceManager<Packet> packetManager;
	const unsigned char* pcapData;
	struct pcap_pkthdr packetHeader;
	Packet* p; 

	pcapData = pcap_next(dev, &packetHeader);
	p = packetManager.getNewInstance();
	p->init((char*)pcapData, packetHeader.caplen, packetHeader.ts, packetHeader.len);
	
	return p;
}

static void startTests()
{
	std::cout << "running tests on ConnectionFilter" << std::endl;
	msg_init();
	msg_setlevel(100);
	captureDevice = pcap_open_offline("data/connectionfiltertest.pcap", errorBuffer);
	if (!captureDevice) {
		BOOST_ERROR(errorBuffer);
	}

	Packet* p;
	ConnectionFilter connFilter(5, 200, 10, 1000);
	
	// first packet is a udp packet
	p = getNextPacket(captureDevice);
	BOOST_REQUIRE(connFilter.processPacket(p) == false);

	// process six packets that come from a connection that did not have any syn packet
	p = getNextPacket(captureDevice);
	BOOST_REQUIRE(connFilter.processPacket(p) == false);
        p = getNextPacket(captureDevice);
        BOOST_REQUIRE(connFilter.processPacket(p) == false);
        p = getNextPacket(captureDevice);
        BOOST_REQUIRE(connFilter.processPacket(p) == false);
        p = getNextPacket(captureDevice);
        BOOST_REQUIRE(connFilter.processPacket(p) == false);
        p = getNextPacket(captureDevice);
        BOOST_REQUIRE(connFilter.processPacket(p) == false);
        p = getNextPacket(captureDevice);
        BOOST_REQUIRE(connFilter.processPacket(p) == false);

	// process a valid short 5 packets connection
        p = getNextPacket(captureDevice);
        BOOST_REQUIRE(connFilter.processPacket(p) == false); // SYN
        p = getNextPacket(captureDevice);
        BOOST_REQUIRE(connFilter.processPacket(p) == false); // SYN
        p = getNextPacket(captureDevice);
        BOOST_REQUIRE(connFilter.processPacket(p) == true);
        p = getNextPacket(captureDevice);
        BOOST_REQUIRE(connFilter.processPacket(p) == false); // FIN
        p = getNextPacket(captureDevice);
        BOOST_REQUIRE(connFilter.processPacket(p) == false); // RST
	
	// process a valid connection
	// ignore the first syn packet
        p = getNextPacket(captureDevice);
        BOOST_REQUIRE(connFilter.processPacket(p) == false); // SYN
        p = getNextPacket(captureDevice);
        BOOST_REQUIRE(connFilter.processPacket(p) == false); // SYN
        p = getNextPacket(captureDevice);
        BOOST_REQUIRE(connFilter.processPacket(p) == true);
        p = getNextPacket(captureDevice);
        BOOST_REQUIRE(connFilter.processPacket(p) == true);
        p = getNextPacket(captureDevice);
        BOOST_REQUIRE(connFilter.processPacket(p) == true);
        p = getNextPacket(captureDevice);
        BOOST_REQUIRE(connFilter.processPacket(p) == false); // passed export limit
        p = getNextPacket(captureDevice);
        BOOST_REQUIRE(connFilter.processPacket(p) == false);
        p = getNextPacket(captureDevice);
        BOOST_REQUIRE(connFilter.processPacket(p) == false); // FIN
        p = getNextPacket(captureDevice);
        BOOST_REQUIRE(connFilter.processPacket(p) == false); // FIN
	p = getNextPacket(captureDevice);
	BOOST_REQUIRE(connFilter.processPacket(p) == false); // ACK
 

	pcap_close(captureDevice);

	std::cout << "All tests on ConnectionFilter passed" << std::endl;
}

#endif
