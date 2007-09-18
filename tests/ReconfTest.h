#ifndef RECONFTEST_H_
#define RECONFTEST_H_


#include "reconf/ConnectionQueue.h"
#include "reconf/FilterModule.h"
#include "sampler/PacketSink.h"
#include "concentrator/IpfixAggregator.hpp"
#include "common/InstanceManager.h"

using boost::shared_ptr;

class ReconfTest
{
	public:
		ReconfTest();
		~ReconfTest();
		
		void normalTest();
		
	private:
		void setup(bool express);
		void sendPacketsTo(Destination<Packet*>* dest, size_t numpackets);
		void shutdown();
};



#endif /*RECONFTEST_H_*/
