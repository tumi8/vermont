#ifndef RECONFTEST_H_
#define RECONFTEST_H_


#include "reconf/ConnectionQueue.h"
#include "reconf/FilterModule.h"
#include "concentrator/IpfixAggregator.hpp"
#include "cfg/InstanceManager.h"

using boost::shared_ptr;

class ReconfTest
{
public:
	ReconfTest();
	~ReconfTest();

	void normalTest();
	void splitterTest();

private:
	void setup(bool express);
	void sendPacketsTo(Destination<Packet*>* dest, size_t numpackets);
	void shutdown();
	
	static InstanceManager<Packet> packetManager;
};



#endif /*RECONFTEST_H_*/
