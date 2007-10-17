#include "BaseAggregator.h"

#include "BaseHashtable.h"

#include <time.h>

/**
 * Creates a new Aggregator.
 * WARNING: call buildAggregator to initialize this module correctly
 * @param pollinterval sets the interval of polling the hashtable for expired flows in ms
 */
BaseAggregator::BaseAggregator(uint32_t pollinterval)
	: rules(0),
	  thread(BaseAggregator::threadWrapper),	  
	  pollInterval(pollinterval)
{
	
}

/**
 * Frees memory used by an Aggregator.
 * Make sure the Aggregator is not being used before calling this method.
 */
BaseAggregator::~BaseAggregator()
{
	// for a strange case a 'delete hashtable' in Rule doesn't work, because
	// it seems we have a cyclic dependency and the compiler complains, so delete it here
	for (size_t i = 0; i < rules->count; i++) {
		delete rules->rule[i]->hashtable;
	}
	delete rules;
}


/**
 * starts the hashtable's thread
 */
void BaseAggregator::performStart()
{
	DPRINTF("called");
	ASSERT(rules != 0, "aggregator must be initialized using function buildAggregator first!");
	
	thread.run(this);
}


/**
 * waits for the hashtable's thread
 */
void BaseAggregator::performShutdown()
{
	thread.join();
}


/**
 * initializes aggregator module and creates hashtable
 * @param rulefile filename with rules to import
 * @param minBufferTime minimum buffer time for flows in hashtable
 * @param maxBufferTime maximum buffer time for flows in hashtable
 */
void BaseAggregator::buildAggregator(char* rulefile, uint16_t minBufferTime, uint16_t maxBufferTime)
{
	Rules* rules = new Rules(rulefile);

	if (!rules) {
		THROWEXCEPTION("could not parse rules file %s", rulefile);
	}
	buildAggregator(rules, minBufferTime, maxBufferTime);
}


/**
 * initializes aggregator module and creates hashtable
 * @param rules rules to use for creation of hashtables
 * @param minBufferTime minimum buffer time for flows in hashtable
 * @param maxBufferTime maximum buffer time for flows in hashtable
 */
void BaseAggregator::buildAggregator(Rules* rules, uint16_t minBufferTime, uint16_t maxBufferTime)
{
	this->rules = rules;

	for (size_t i = 0; i < rules->count; i++) {
		rules->rule[i]->initialize();
		rules->rule[i]->hashtable = createHashtable(rules->rule[i], minBufferTime, maxBufferTime);
	}

	msg(MSG_INFO, "Done. Parsed %d rules; minBufferTime %d, maxBufferTime %d", rules->count, minBufferTime, maxBufferTime);
}


/**
 * thread which regularly scans hashtable for expired buckets/flows
 */
void BaseAggregator::exporterThread()
{	
	timespec req;
	req.tv_sec = pollInterval / 1000;
	req.tv_nsec = (pollInterval % 1000) * 1000;
	
	msg(MSG_INFO, "Polling aggregator each %u msec", pollInterval);	
	while (!exitFlag) {
		// restart nanosleep with the remaining sleep time
		// if we got interrupted by a signal
		while (nanosleep(&req, &req) == -1 && errno == EINTR);
		
		mutex.lock();
		for (size_t i = 0; i < rules->count; i++) {
			rules->rule[i]->hashtable->expireFlows();
		}
		mutex.unlock();
	}
}


/**
 * small static wrapper function to start thread
 */
void* BaseAggregator::threadWrapper(void* instance)
{
	BaseAggregator* agg = reinterpret_cast<BaseAggregator*>(instance);
	agg->exporterThread();	
	return 0;
}
