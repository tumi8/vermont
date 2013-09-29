#ifdef HAVE_CONNECTION_FILTER

#include "ConnectionFilterTest.h"

#include <modules/packet/filter//ConnectionFilter.h>
#include <modules/packet/Packet.h>
#include <core/InstanceManager.h>
#include <common/msg.h>

#include <pcap.h>
#include <iostream>


static pcap_t *captureDevice;
static char errorBuffer[PCAP_ERRBUF_SIZE]; 


ConnectionFilterTestSuite::ConnectionFilterTestSuite()
{
}

static Packet* getNextPacket(pcap_t* dev)
{
	static InstanceManager<Packet> packetManager("Packet");
	const unsigned char* pcapData;
	struct pcap_pkthdr packetHeader;
	Packet* p; 

	pcapData = pcap_next(dev, &packetHeader);
	p = packetManager.getNewInstance();
	p->init((char*)pcapData, packetHeader.caplen, packetHeader.ts, 0, packetHeader.len, DLT_EN10MB);
	
	return p;
}

Test::TestResult ConnectionFilterTestSuite::execTest()
{
	std::cout << "running tests on ConnectionFilter" << std::endl;
	msg_init();
	msg_setlevel(100);
	captureDevice = pcap_open_offline("data/connectionfiltertest.pcap", errorBuffer);
	if (!captureDevice) {
		ERROR(errorBuffer);
	}

	Packet* p;
	ConnectionFilter connFilter(5, 100, 10, 1000);
	
	// first packet is a udp packet
	p = getNextPacket(captureDevice);
	REQUIRE(connFilter.processPacket(p) == false);

	// process six packets that come from a connection that did not have any syn packet
	p = getNextPacket(captureDevice);
	REQUIRE(connFilter.processPacket(p) == false);
        p = getNextPacket(captureDevice);
        REQUIRE(connFilter.processPacket(p) == false);
        p = getNextPacket(captureDevice);
        REQUIRE(connFilter.processPacket(p) == false);
        p = getNextPacket(captureDevice);
        REQUIRE(connFilter.processPacket(p) == false);
        p = getNextPacket(captureDevice);
        REQUIRE(connFilter.processPacket(p) == false);
        p = getNextPacket(captureDevice);
        REQUIRE(connFilter.processPacket(p) == false);

	// process a valid short 5 packets connection
        p = getNextPacket(captureDevice);
        REQUIRE(connFilter.processPacket(p) == false); // SYN
        p = getNextPacket(captureDevice);
        REQUIRE(connFilter.processPacket(p) == false); // SYN
        p = getNextPacket(captureDevice);
        REQUIRE(connFilter.processPacket(p) == true);
        p = getNextPacket(captureDevice);
        REQUIRE(connFilter.processPacket(p) == false); // FIN
        p = getNextPacket(captureDevice);
        REQUIRE(connFilter.processPacket(p) == false); // RST
	
	// process a valid connection
	// ignore the first syn packet
        p = getNextPacket(captureDevice);
        REQUIRE(connFilter.processPacket(p) == false); // SYN
        p = getNextPacket(captureDevice);
        REQUIRE(connFilter.processPacket(p) == false); // SYN
        p = getNextPacket(captureDevice);
        REQUIRE(connFilter.processPacket(p) == true);
        p = getNextPacket(captureDevice);
        REQUIRE(connFilter.processPacket(p) == true);
        p = getNextPacket(captureDevice);
        REQUIRE(connFilter.processPacket(p) == true);
        p = getNextPacket(captureDevice);
        REQUIRE(connFilter.processPacket(p) == false); // passed export limit
        p = getNextPacket(captureDevice);
        REQUIRE(connFilter.processPacket(p) == false);
        p = getNextPacket(captureDevice);
        REQUIRE(connFilter.processPacket(p) == false); // FIN
        p = getNextPacket(captureDevice);
        REQUIRE(connFilter.processPacket(p) == false); // FIN
	p = getNextPacket(captureDevice);
	REQUIRE(connFilter.processPacket(p) == false); // ACK

	pcap_close(captureDevice);

	std::cout << "All tests on ConnectionFilter passed" << std::endl;

	return PASSED;
}

#endif
