/*
 * Vermont Testsuite
 * Copyright (C) 2007 Tobias Limmer <http://www7.informatik.uni-erlangen.de/~limmer>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "AggregationPerfTest.h"

#include "common/Time.h"
#include "TestSuiteBase.h"
#include "TestQueue.h"
#include "core/Module.h"
#include "core/ThreadSafeAdapter.h"
#include "core/ConnectionQueue.h"
#include "modules/ipfix/aggregator/PacketAggregator.h"
#include "CounterDestination.h"

#include <sys/time.h>
#include <time.h>

InstanceManager<Packet> AggregationPerfTest::packetManager("Packet");

/**
 * @param fast determines if this test should be performed really fast or slower for performance measurements
 */
AggregationPerfTest::AggregationPerfTest(bool fast)
{
	if (fast) {
		numPackets = 1000;
	} else {
		numPackets = 500000;
	}
}

AggregationPerfTest::~AggregationPerfTest()
{

}


Rule::Field* AggregationPerfTest::createRuleField(const std::string& typeId)
{
	Rule::Field* ruleField = new Rule::Field();
	ruleField->modifier = Rule::Field::KEEP;
	const ipfix_identifier* ipfixid = ipfix_name_lookup(typeId.c_str());
	ruleField->type.id = ipfixid->id;
	REQUIRE(ruleField->type.id != 0);
	ruleField->type.length = ipfixid->length;
	REQUIRE(ruleField->type.length != 0);
	if ((ruleField->type.id==IPFIX_TYPEID_sourceIPv4Address)
			|| (ruleField->type.id == IPFIX_TYPEID_destinationIPv4Address)) {
		ruleField->type.length++;
	}

	return ruleField;
}


Rules* AggregationPerfTest::createRules()
{
	Rule* rule = new Rule();
	rule->id = 1111;
	const char* rulefields[] = { "sourceipv4address", "destinationipv4address", "destinationtransportport",
								  "sourcetransportport", "packetdeltacount", "octetdeltacount",
								  "flowstartseconds", "flowendseconds", "protocolidentifier", 0 };

	for (int i=0; rulefields[i] != 0; i++) {
		rule->field[rule->fieldCount++] = createRuleField(rulefields[i]);
	}

	Rules* rules = new Rules();
	rules->rule[0] = rule;
	rules->count = 1;

	return rules;
}

Test::TestResult AggregationPerfTest::execTest()
{

	// create a packet sampler which lets only half of the packets through
	// NOTICE: the sampler will be destroyed by the d'tor of FilterModule

	ConnectionQueue<Packet*> queue1(10);
	TestQueue<IpfixRecord*> tqueue;

	PacketAggregator agg(1);
	Rules* rules = createRules();
	agg.buildAggregator(rules, 0, 0, 16);

	queue1.connectTo(&agg);
	agg.connectTo(&tqueue);

	agg.start();
	queue1.start();

	struct timeval starttime;
	REQUIRE(gettimeofday(&starttime, 0) == 0);

	sendPacketsTo(&queue1, numPackets);

	// check that at least one record was received
	IpfixRecord* rec;
	ASSERT(tqueue.pop(1000, &rec), "received timeout when should have received flow!");

	struct timeval stoptime;
	REQUIRE(gettimeofday(&stoptime, 0) == 0);
	struct timeval difftime;
	REQUIRE(timeval_subtract(&difftime, &stoptime, &starttime) == 0);
	printf("Aggregator: needed time for processing %d packets: %d.%06d seconds\n", numPackets, (int)difftime.tv_sec, (int)difftime.tv_usec);


	queue1.shutdown();
	agg.shutdown();

	return PASSED;
}


void AggregationPerfTest::sendPacketsTo(Destination<Packet*>* dest, uint32_t numpackets)
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

