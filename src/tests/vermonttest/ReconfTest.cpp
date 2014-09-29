#include "ReconfTest.h"

#include "TestSuiteBase.h"
#include "modules/packet/filter//SystematicSampler.h"
#include "common/msg.h"
#include "CounterDestination.h"
#include "modules/ConfigManager.h"
#include "core/ConnectionSplitter.h"
#include "PrinterModule.h"

#include <unistd.h>

InstanceManager<Packet> ReconfTest::packetManager("Packet");

ReconfTest::ReconfTest()
{
}

ReconfTest::~ReconfTest()
{
}

Test::TestResult ReconfTest::execTest()
{
	normalTest();
	splitterTest();

	return PASSED;
}

void ReconfTest::sendPacketsTo(Destination<Packet*>* dest, size_t numpackets)
{
	unsigned char packetdata[] = { 0x00, 0x12, 0x1E, 0x08, 0xE0, 0x1F, 0x00, 0x15, 0x2C, 0xDB, 0xE4, 0x00,
			0x08, 0x00, 0x45, 0x00, 0x00, 0x2C, 0xEF, 0x42, 0x40, 0x00, 0x3C, 0x06, 0xB3, 0x51,
			0xC3, 0x25, 0x84, 0xBE, 0x5B, 0x20, 0xF9, 0x33, 0x13, 0x8B, 0x07, 0x13, 0x63, 0xF2,
			0xA0, 0x06, 0x2D, 0x07, 0x36, 0x2B, 0x50, 0x18, 0x3B, 0x78, 0x67, 0xC9, 0x00, 0x00,
			0x6F, 0x45, 0x7F, 0x40 };
	unsigned int packetdatalen = 58;

	// just push our sample packet a couple of times into the filter
	struct timeval curtime;
	REQUIRE(gettimeofday(&curtime, 0) == 0);

	for (size_t i = 0; i < numpackets; i++) {
		Packet* packet = packetManager.getNewInstance();
		packet->init((char*)packetdata, packetdatalen, curtime, 0, packetdatalen, 14);
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
	size_t nr_of_packets = 100; // must be a dividable by 2

	// create a packet sampler which lets only half of the packets through
	// NOTICE: the sampler will be destroyed by the d'tor of FilterModule
	SystematicSampler* sampler = new SystematicSampler(SYSTEMATIC_SAMPLER_COUNT_BASED, 2, 1);

	FilterModule filter;
	filter.addProcessor(sampler);

	ConnectionQueue<Packet*> queue1(10);
	ConnectionQueue<Packet*> queue2(20);

	PrinterModule counter;
	counter.doPrint(false);

	queue1.connectTo(&filter);
	filter.connectTo(&queue2);
	queue2.connectTo(&counter);

	queue1.start();
	queue2.start();

	sendPacketsTo(&queue1, nr_of_packets);
	while (queue2.getCount() > 0 || queue1.getCount() > 0) {
		sleep(1);
	}

	ASSERT(counter.getCount() == nr_of_packets/2,
			"The filter hasn't eliminated half of the packets");

	/**
	 * remove the sampler and redo the test.
	 * This time every packet should get through.
	 * __________     __________    _______________
	 * | queue1 | ->  | queue2 |-> | PacketCounter |
	 *  --------      ----------    ---------------
	 */

	queue1.disconnect();
	queue1.connectTo(&queue2);

	counter.reset();

	sendPacketsTo(&queue1, nr_of_packets);
	while (queue2.getCount() > 0|| queue1.getCount() > 0) {
		sleep(1);
	}

	ASSERT(counter.getCount() == nr_of_packets, "Not all packages get through");

	queue1.shutdown();
	queue2.shutdown();
}


/**
 * This creates the following chain:
 *                               _______________  
 *                              | PacketCounter | 
 * __________     __________  /  ---------------  
 * | sampler | -> | splitter | 
 *  --------      ----------  \  _______________  
 *                              | PacketCounter | 
 *                               ---------------  
 */
void ReconfTest::splitterTest()
{
	size_t nr_of_packets = 100; // must be a dividable by 2
	PrinterModule counter1, counter2;
	counter1.doPrint(false);
	counter2.doPrint(false);

	// create a packet sampler which lets only half of the packets through
	// NOTICE: the sampler will be destroyed by the d'tor of FilterModule
	SystematicSampler* sampler = new SystematicSampler(SYSTEMATIC_SAMPLER_COUNT_BASED, 2, 1);

	FilterModule filter;
	filter.addProcessor(sampler);

	ConnectionSplitter<FilterModule::src_value_type>* splitter;
	splitter = new ConnectionSplitter<FilterModule::src_value_type>();
	
	filter.connectTo(splitter);
	splitter->connectTo(&counter1);
	splitter->connectTo(&counter2);

	sendPacketsTo(&filter, nr_of_packets);

	ASSERT(counter1.getCount() == nr_of_packets/2,
			"The filter hasn't eliminated half of the packets for counter1");
	ASSERT(counter2.getCount() == nr_of_packets/2,
			"The filter hasn't eliminated half of the packets for counter2");

	/**
	 * remove the sampler and redo the test.
	 * This time every packet should get through.
	 * __________      _______________
	 * | sampler| ->  | PacketCounter |
	 *  --------       ---------------
	 */
	
	filter.disconnect();
	
	counter1.reset();
	counter2.reset();

	filter.connectTo(&counter1);
	sendPacketsTo(&filter, nr_of_packets);
	ASSERT(counter1.getCount() == nr_of_packets/2, "Not all packages get through");
	ASSERT(counter2.getCount() == 0, "splitter disconnect failed");

	// cleanup
	delete splitter;
}
