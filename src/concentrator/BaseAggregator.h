#ifndef BASEAGGREGATOR_H_
#define BASEAGGREGATOR_H_

#include "Rules.hpp"
#include "core/Module.h"
#include "common/Mutex.h"

#include <stdint.h>


class BaseAggregator 
	: public Module, public Source<IpfixRecord*>
{
public:
	BaseAggregator(uint32_t pollinterval);
	virtual ~BaseAggregator();
		
	void buildAggregator(Rules* rules, uint16_t minBufferTime, uint16_t maxBufferTime, uint8_t hashbits);
	void buildAggregator(char* rulefile, uint16_t minBufferTime, uint16_t maxBufferTime, uint8_t hashbits);

	// events from Module
	virtual void preReconfiguration();
	virtual void onReconfiguration1();
	virtual void postReconfiguration();

	virtual string getStatisticsXML(double interval);

protected:
	Rules* rules; /**< Set of rules that define the aggregator */
	Mutex mutex; /**< ensures that exporterThread does not interfere with aggregation of incoming flows */
	
	/**
	 * creates a hashtable using the given parameters
	 * to be overloaded by specialised versions of aggregator module
	 */
	virtual BaseHashtable* createHashtable(Rule* rule, uint16_t minBufferTime, uint16_t maxBufferTime, uint8_t hashbits) = 0;
	void poll();
	void exporterThread();
	
	// events from Module
	virtual void performStart();
	virtual void performShutdown();
	
private:
	Thread thread;
	uint32_t pollInterval; /**< polling interval in milliseconds */
	
	static void* threadWrapper(void* instance);
};

#endif /*BASEAGGREGATOR_H_*/
