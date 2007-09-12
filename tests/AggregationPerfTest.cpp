#include "AggregationPerfTest.h"

#include "sampler/ExpressHookingFilter.h"
#include "sampler/Filter.h"
#include "common/Time.h"
#include "test.h"

#include <sys/time.h>
#include <time.h>


/**
 * @param fast determines if this test should be performed really fast or slower for performance measurements
 */
AggregationPerfTest::AggregationPerfTest(bool fast)
{
	if (fast) {
		numPackets = 3000;
	} else {
		numPackets = 500000;
	}

}

AggregationPerfTest::~AggregationPerfTest()
{
	delete filter;
	delete packetSink;
	delete ipfixAggregator;
	delete packetManager;
}

void AggregationPerfTest::normalTest()
{
	setup(false);
	struct timeval starttime;
	REQUIRE(gettimeofday(&starttime, 0) == 0);
	start(numPackets);
	struct timeval stoptime;
	REQUIRE(gettimeofday(&stoptime, 0) == 0);
	struct timeval difftime;
	REQUIRE(timeval_subtract(&difftime, &stoptime, &starttime) == 0);

	printf("Aggregator: needed time for processing %d packets: %d.%06d seconds\n", numPackets, (int)difftime.tv_sec, (int)difftime.tv_usec);

	shutdown();
}

void AggregationPerfTest::expressTest()
{
	setup(true);
	struct timeval starttime;
	REQUIRE(gettimeofday(&starttime, 0) == 0);
	start(numPackets);
	struct timeval stoptime;
	REQUIRE(gettimeofday(&stoptime, 0) == 0);
	struct timeval difftime;
	REQUIRE(timeval_subtract(&difftime, &stoptime, &starttime) == 0);

	printf("ExpressAggregator: needed time for processing %d packets: %d.%06d seconds\n", numPackets, (int)difftime.tv_sec, (int)difftime.tv_usec);

	shutdown();
}

Rule::Field* AggregationPerfTest::createRuleField(const string& typeId)
{
	Rule::Field* ruleField = new Rule::Field();
	ruleField->modifier = Rule::Field::KEEP;
	ruleField->type.id = string2typeid(typeId.c_str());
	REQUIRE(ruleField->type.id != 0);
	ruleField->type.length = string2typelength(typeId.c_str());
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

void AggregationPerfTest::setup(bool express)
{
	packetSink = new PacketSink();

	packetManager = new InstanceManager<Packet>();

	Rules* rules = createRules();
	int inactiveBufferTime = 5;   // maximum number of seconds until non-active flows are exported
	int activeBufferTime = 10;   // maximum number of seconds until active flows are exported
	// note: we do not need to specify any receiving module for the ipfixaggregator,
	// as deconstruction of unused instances is done with shared pointers
	ipfixAggregator = new IpfixAggregator(rules, inactiveBufferTime, activeBufferTime);

	if (express) {
		hookingFilter = new ExpressHookingFilter(ipfixAggregator);
	} else {
		hookingFilter = new HookingFilter(ipfixAggregator);
	}

	filter = new Filter();
	filter->addProcessor(hookingFilter);
	filter->setReceiver(packetSink);

	// start all needed threads
	packetSink->runSink();
	ipfixAggregator->runSink();
	ipfixAggregator->start();
	filter->startFilter();
}

void AggregationPerfTest::shutdown()
{
	TimeoutSemaphore::shutdown();
	filter->terminate();
	packetSink->terminateSink();
	ipfixAggregator->terminateSink();
	TimeoutSemaphore::restart();
}


void AggregationPerfTest::start(unsigned int numpackets)
{
	char packetdata[] = { 0x00, 0x12, 0x1E, 0x08, 0xE0, 0x1F, 0x00, 0x15, 0x2C, 0xDB, 0xE4, 0x00, 0x08, 0x00, 0x45, 0x00, 
						  0x00, 0x2C, 0xEF, 0x42, 0x40, 0x00, 0x3C, 0x06, 0xB3, 0x51, 0xC3, 0x25, 0x84, 0xBE, 0x5B, 0x20, 
						  0xF9, 0x33, 0x13, 0x8B, 0x07, 0x13, 0x63, 0xF2, 0xA0, 0x06, 0x2D, 0x07, 0x36, 0x2B, 0x50, 0x18, 
						  0x3B, 0x78, 0x67, 0xC9, 0x00, 0x00, 0x6F, 0x45, 0x7F, 0x40 };
	unsigned int packetdatalen = 58;

	// just push our sample packet a couple of times into the filter
	struct timeval curtime;
	REQUIRE(gettimeofday(&curtime, 0) == 0);

	ConcurrentQueue<Packet*>* filterq = filter->getQueue();
	for (unsigned int i=0; i<numpackets; i++) {
		Packet* p = packetManager->getNewInstance();
		p->init((char*)packetdata, packetdatalen, curtime);
		filterq->push(p);
	}
}

