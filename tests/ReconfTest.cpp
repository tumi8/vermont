#include "ReconfTest.h"
#include "sampler/SystematicSampler.h"
#include "common/PacketInstanceManager.h"

class PacketCounter: public Destination<Packet*>
{
public:
	PacketCounter() : count(0) { }
	
	virtual void receive(Packet* packet)
	{
		packet->removeReference();
		count++;
	}

	unsigned int getCount()
	{
		return count;
	}
	
private:
	unsigned int count;
};


ReconfTest::ReconfTest()
{
}

ReconfTest::~ReconfTest()
{
}

void ReconfTest::sendPacketsTo(Destination<Packet*>* dest, size_t numpackets)
{
	char packetdata[] = { 0x00, 0x12, 0x1E, 0x08, 0xE0, 0x1F, 0x00, 0x15, 0x2C, 0xDB, 0xE4, 0x00, 0x08, 0x00, 0x45, 0x00, 
			      0x00, 0x2C, 0xEF, 0x42, 0x40, 0x00, 0x3C, 0x06, 0xB3, 0x51, 0xC3, 0x25, 0x84, 0xBE, 0x5B, 0x20, 
			      0xF9, 0x33, 0x13, 0x8B, 0x07, 0x13, 0x63, 0xF2, 0xA0, 0x06, 0x2D, 0x07, 0x36, 0x2B, 0x50, 0x18, 
			      0x3B, 0x78, 0x67, 0xC9, 0x00, 0x00, 0x6F, 0x45, 0x7F, 0x40 };
	unsigned int packetdatalen = 58;

	// just push our sample packet a couple of times into the filter
	struct timeval curtime;
	BOOST_REQUIRE(gettimeofday(&curtime, 0) == 0);

	for (size_t i = 0; i < numpackets; i++) {
		Packet* packet = PacketInstanceManager::getManager()->getNewInstance();
		packet->init((char*)packetdata, packetdatalen, curtime);
		dest->receive(packet);
	}
}

/**
 * This creates the following chain:
 * __________     __________     ________     _______________
 * | queue1 | -> | sampler | -> | queue2 |-> | PacketCounter |
 *  --------      ----------    ----------    ---------------
 */
void ReconfTest::normalTest() 
{
	
	// create a packet sampler which lets only half of the packets through
	SystematicSampler sampler = SystematicSampler(SYSTEMATIC_SAMPLER_COUNT_BASED, 1, 1);

	FilterModule filter;
	filter.addProcessor(&sampler);

	ConnectionQueue<Packet*> queue1(10);
	ConnectionQueue<Packet*> queue2(20);
	
	PacketCounter counter;
	
	queue1.connectTo(&filter);
	filter.connectTo(&queue2);
	queue2.connectTo(&counter);

	sendPacketsTo(&queue1, 100);
	printf("the last module got %u packets\n", counter.getCount());

	BOOST_CHECK(1 == 2);
}

