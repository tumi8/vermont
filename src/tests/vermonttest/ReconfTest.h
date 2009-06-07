#ifndef RECONFTEST_H_
#define RECONFTEST_H_


#include "core/ConnectionQueue.h"
#include "modules/packet/filter/FilterModule.h"
#include "modules/ipfix/aggregator/IpfixAggregator.hpp"
#include "core/InstanceManager.h"

#include "TestSuiteBase.h"

using boost::shared_ptr;

class ReconfTest : public Test
{
public:
	ReconfTest();
	~ReconfTest();

	virtual TestResult execTest();
private:
	void normalTest();
	void splitterTest();

	void setup(bool express);
	void sendPacketsTo(Destination<Packet*>* dest, size_t numpackets);
	void shutdown();
	
	static InstanceManager<Packet> packetManager;
};



#endif /*RECONFTEST_H_*/
