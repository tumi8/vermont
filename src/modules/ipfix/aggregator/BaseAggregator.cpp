/*
 * Vermont Aggregator Subsystem
 * Copyright (C) 2009 Vermont Project
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

#include "BaseAggregator.h"

#include "BaseHashtable.h"
#include "common/Time.h"

#include <time.h>
#include <sstream>

/**
 * Creates a new Aggregator.
 * WARNING: call buildAggregator to initialize this module correctly
 * @param pollinterval sets the interval of polling the hashtable for expired flows in ms
 */
BaseAggregator::BaseAggregator(uint32_t pollinterval)
	: rules(0),
	  thread(BaseAggregator::threadWrapper, "BaseAggregator"),
	  pollInterval(pollinterval)
{

}

/**
 * Frees memory used by an Aggregator.
 * Make sure the Aggregator is not being used before calling this method.
 */
BaseAggregator::~BaseAggregator()
{
	// to make sure that exitFlag is set and performShutdown() is called
	shutdown(false);

	// for a strange case a 'delete hashtable' in Rule doesn't work, because
	// it seems we have a cyclic dependency and the compiler complains, so delete it here
	for (size_t i = 0; i < rules->count; i++) {
		delete rules->rule[i]->hashtable;
	}
	delete rules;
}


/**
 * starts the hashtable's thread and notifies hashtables about startup
 */
void BaseAggregator::performStart()
{
	DPRINTF("called");
	ASSERT(rules != 0, "aggregator must be initialized using function buildAggregator first!");

	// notify all hashtables that start is in progress
	for (uint32_t i=0; i<rules->count; i++) {
		rules->rule[i]->hashtable->performStart();
	}

	thread.run(this);
}


/**
 * waits for the hashtable's thread and notifies hashtables
 */
void BaseAggregator::performShutdown()
{
	for (uint32_t i=0; i<rules->count; i++) {
		rules->rule[i]->hashtable->performShutdown();
	}

	connected.shutdown();
	thread.join();
}

/**
 * notifies all hashtables about imminent reconfiguration
 */
void BaseAggregator::preReconfiguration()
{
	for (uint32_t i=0; i<rules->count; i++) {
		rules->rule[i]->hashtable->preReconfiguration();
	}
}

/**
 * notifies hashtables about reconfiguration
 */
void BaseAggregator::onReconfiguration1()
{
	for (uint32_t i=0; i<rules->count; i++) {
		rules->rule[i]->hashtable->onReconfiguration1();
	}
}

/**
 * notifies hashtables about reconfiguration
 */
void BaseAggregator::postReconfiguration()
{
	for (uint32_t i=0; i<rules->count; i++) {
		rules->rule[i]->hashtable->postReconfiguration();
	}
}


/**
 * initializes aggregator module and creates hashtable
 * @param rulefile filename with rules to import
 * @param minBufferTime minimum buffer time for flows in hashtable
 * @param maxBufferTime maximum buffer time for flows in hashtable
 */
void BaseAggregator::buildAggregator(char* rulefile, uint16_t minBufferTime, uint16_t maxBufferTime, uint8_t hashbits)
{
	Rules* rules = new Rules(rulefile);

	if (!rules) {
		THROWEXCEPTION("could not parse rules file %s", rulefile);
	}
	buildAggregator(rules, minBufferTime, maxBufferTime, hashbits);
}


/**
 * initializes aggregator module and creates hashtable
 * @param rules rules to use for creation of hashtables
 * @param minBufferTime minimum buffer time for flows in hashtable
 * @param maxBufferTime maximum buffer time for flows in hashtable
 */
void BaseAggregator::buildAggregator(Rules* rules, uint16_t minBufferTime, uint16_t maxBufferTime, uint8_t hashbits)
{
	this->rules = rules;

	for (size_t i = 0; i < rules->count; i++) {
		rules->rule[i]->initialize();
		rules->rule[i]->hashtable = createHashtable(rules->rule[i], minBufferTime, maxBufferTime, hashbits);
	}

	msg(MSG_INFO, "Done. Parsed %d rules; minBufferTime %d, maxBufferTime %d", rules->count, minBufferTime, maxBufferTime);
}


/**
 * thread which regularly scans hashtable for expired buckets/flows
 */
void BaseAggregator::exporterThread()
{
	struct timeval inttimer;
	gettimeofday(&inttimer, 0);
	//struct timeval difftime;
	//REQUIRE(timeval_subtract(&difftime, &stoptime, &starttime) == 0);


	/*timespec req;
	req.tv_sec = pollInterval / 1000;
	req.tv_nsec = (pollInterval % 1000) * 1000;*/


	registerCurrentThread();

	msg(MSG_INFO, "Polling aggregator each %u msec", pollInterval);
	while (!exitFlag) {
		addToCurTime(&inttimer, pollInterval);

		struct timeval curtime;
		gettimeofday(&curtime, 0);

		struct timeval difftime;
		if (timeval_subtract(&difftime, &inttimer, &curtime)!=1) {
			// restart nanosleep with the remaining sleep time
			// if we got interrupted by a signal
			struct timespec ts;
			TIMEVAL_TO_TIMESPEC(&difftime, &ts);
			while (nanosleep(&ts, &ts) == -1 && errno == EINTR);
		}

		gettimeofday(&curtime, 0);
		DPRINTFL(MSG_VDEBUG,"Aggregator: starting Export");
		for (size_t i = 0; i < rules->count; i++) {
			rules->rule[i]->hashtable->expireFlows();
		}
		struct timeval endtime;
		gettimeofday(&endtime, 0);
		timeval_subtract(&difftime, &endtime, &curtime);

		DPRINTFL(MSG_VDEBUG,"Aggregator: export took %.03f secs", (float)difftime.tv_usec/1000000+difftime.tv_sec);
	}

	if (getShutdownProperly()) {
		for (size_t i = 0; i < rules->count; i++) {
			rules->rule[i]->hashtable->expireFlows(true);
		}
	}

	unregisterCurrentThread();
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

string BaseAggregator::getStatisticsXML(double interval)
{
	ostringstream oss;
	for (size_t i=0; i<rules->count; i++) {
		oss << "<hashtable rule=\"" << i << "\">";
		oss << rules->rule[i]->hashtable->getStatisticsXML(interval);
		oss << "</hashtable>";
	}
	return oss.str();
}
