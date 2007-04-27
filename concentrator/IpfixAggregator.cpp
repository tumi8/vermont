/*
 * IPFIX Concentrator Module Library
 * Copyright (C) 2004 Christoph Sommer <http://www.deltadevelopment.de/users/christoph/ipfix/>
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

#include <stdexcept>
#include <netinet/in.h>
#include <unistd.h>
#include "IpfixAggregator.hpp"
#include "IpfixParser.hpp"
#include "IpfixSender.hpp"

#include "msg.h"

/**
 * Creates a new Aggregator. Do not forget to set the callback functions, then call @c startAggregator().
 * @param ruleFile filename of file containing a set of rules
 * @param minBufferTime TODO
 * @param maxBufferTime TODO
 */
IpfixAggregator::IpfixAggregator(char* ruleFile, uint16_t minBufferTime, uint16_t maxBufferTime)
{
	Rules* rules = new Rules(ruleFile);

	if (!rules) {
		throw std::runtime_error(std::string("Aggregator: could not parse rules file ") + std::string(ruleFile));
	}
	buildAggregator(rules, minBufferTime, maxBufferTime);
}

/**
 * Creates a new Aggregator. Do not forget to set the callback functions, then call @c startAggreagtor().
 * @param rules Rules for aggregator to work with
 * @param minBufferTime TODO
 * @param maxBufferTime TODO
 */
IpfixAggregator::IpfixAggregator(Rules* rules, uint16_t minBufferTime, uint16_t maxBufferTime)
{
	buildAggregator(rules, minBufferTime, maxBufferTime);
}

/**
 * Builds a new aggregator from the given rules (helper function for @c createAggregator and @c createAggregatorFromRules)
 */
void IpfixAggregator::buildAggregator(Rules* rules, uint16_t minBufferTime, uint16_t maxBufferTime)
{
	int i;

	this->rules = rules;

	for (i = 0; i < rules->count; i++) {
		rules->rule[i]->hashtable = new Hashtable(rules->rule[i], minBufferTime, maxBufferTime);
	}

	if (pthread_mutex_init(&mutex, NULL) != 0) {
		msg(MSG_FATAL, "Could not init mutex");
	}
		
	if (pthread_mutex_lock(&mutex) != 0) {
		msg(MSG_FATAL, "Could not lock mutex");
	}

	msg(MSG_INFO, "Aggregator: Done. Parsed %d rules; minBufferTime %d, maxBufferTime %d", rules->count, minBufferTime, maxBufferTime);
}

/**
 * Frees memory used by an Aggregator.
 * Make sure the Aggregator is not being used before calling this method.
 */
IpfixAggregator::~IpfixAggregator()
{
	int i;
	for (i = 0; i < rules->count; i++) {
		delete ((Hashtable*)rules->rule[i]->hashtable);
	}
	delete rules;

	pthread_mutex_unlock(&mutex);
	pthread_mutex_destroy(&mutex);
}

/**
 * Starts or resumes processing Records
 */
void IpfixAggregator::start() {
	pthread_mutex_unlock(&mutex);
}

/**
 * Temporarily pauses processing Records
 */
void IpfixAggregator::stop() {
	pthread_mutex_lock(&mutex);
}

/**
 * Injects new DataRecords into the Aggregator.
 * @param sourceID ignored
 * @param ti structure describing @c data
 * @param length length (in bytes) of @c data
 * @param data raw data block containing the Record
 * @return 0 on success, non-zero on error
 */
int IpfixAggregator::onDataRecord(IpfixRecord::SourceID* sourceID, IpfixRecord::TemplateInfo* ti, uint16_t length, IpfixRecord::Data* data)
{
	int i;
	DPRINTF("aggregateDataRecord: Got a Data Record\n");

	if(!rules) {
		msg(MSG_FATAL, "Aggregator not started");
		return -1;
	}

	pthread_mutex_lock(&mutex);
	for (i = 0; i < rules->count; i++) {
		if (rules->rule[i]->templateDataMatches(ti, data)) {
			DPRINTF("rule %d matches", i);

			((Hashtable*)rules->rule[i]->hashtable)->aggregateTemplateData(ti, data);
		}
	}
	pthread_mutex_unlock(&mutex);

	return 0;
}

/**
 * Injects new DataRecords into the Aggregator.
 * @param sourceID ignored
 * @param ti structure describing @c data
 * @param length length (in bytes) of @c data
 * @param data raw data block containing the Record
 * @return 0 on success, non-zero on error
 */
int IpfixAggregator::onDataDataRecord(IpfixRecord::SourceID* sourceID, IpfixRecord::DataTemplateInfo* ti, uint16_t length, IpfixRecord::Data* data)
{
	int i;
	DPRINTF("aggregateDataDataRecord: Got a DataData Record\n");

	if(!rules) {
		msg(MSG_FATAL, "Aggregator not started");
		return -1;
	}

	pthread_mutex_lock(&mutex);
	for (i = 0; i < rules->count; i++) {
		if (rules->rule[i]->dataTemplateDataMatches(ti, data)) {
			DPRINTF("rule %d matches\n", i);
			((Hashtable*)rules->rule[i]->hashtable)->aggregateDataTemplateData(ti, data);
		}
	}
	pthread_mutex_unlock(&mutex);

	return 0;
}

/**
 * Checks for flows buffered longer than @c ipfixAggregator::minBufferTime and/or @c ipfixAggregator::maxBufferTime and passes them to the previously defined callback functions.
 */
void IpfixAggregator::poll() {
	int i;
	pthread_mutex_lock(&mutex);
	for (i = 0; i < rules->count; i++) {
		((Hashtable*)rules->rule[i]->hashtable)->expireFlows();
	}
	pthread_mutex_unlock(&mutex);
}

/**
 * Adds a set of callback functions to the list of functions to call when Templates or Records have to be sent
 * @param flowSink the destination module
 */
void IpfixAggregator::addFlowSink(FlowSink* flowSink) {
	int i;
	for (i = 0; i < rules->count; i++) {
		((Hashtable*)rules->rule[i]->hashtable)->addFlowSink(flowSink);
	}
}


/**
 * Called by the logger timer thread. Dumps info using msg_stat
 */
void IpfixAggregator::stats()
{
	int i;

	pthread_mutex_lock(&mutex);
	for (i = 0; i < rules->count; i++) {
		int j;
		uint32_t usedBuckets = 0;
		uint32_t usedHeads = 0;
		uint32_t longestSpillchain = 0;
		uint32_t avgAge = 0;

		Hashtable* ht = rules->rule[i]->hashtable;
		msg_stat("Concentrator: Rule %2d: Records: %6d received, %6d sent", i, ht->recordsReceived, ht->recordsSent);
		ht->recordsReceived = 0;
		ht->recordsSent = 0;

		for (j = 0; j < HASHTABLE_SIZE; j++) {
			Hashtable::Bucket* hb = ht->buckets[j];
			if (hb) usedHeads++;

			uint32_t bucketsInSpillchain = 0;
			while (hb) {
				avgAge += time(0) - (hb->forceExpireTime - ht->maxBufferTime);
				usedBuckets++;
				bucketsInSpillchain++;
				hb = hb->next;
			}
			if (bucketsInSpillchain > longestSpillchain) longestSpillchain = bucketsInSpillchain;
		}

		msg_stat("Concentrator: Rule %2d: Hashbuckets: %6d used, %6d at head, %6d max chain, %6d avg age", i, usedBuckets, usedHeads, longestSpillchain, usedBuckets?(avgAge / usedBuckets):0);
	}
	pthread_mutex_unlock(&mutex);
}
