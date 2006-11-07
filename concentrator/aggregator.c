/** \file
 * Aggregator module.
 * Uses rules.c and hashing.c to implement an IPFIX Aggregator.
 * Call createAggregator() to instantiate an aggregator,
 * start it with startAggregator(), feed it with aggregateDataRecord(),
 * poll for expired flows with pollAggregator().
 * To destroy the Aggregator, first stop it with stopAggregator(), then
 * call destroyAggregator().
 */

#include <netinet/in.h>
#include <unistd.h>
#include "aggregator.h"
#include "rcvIpfix.h"
#include "sndIpfix.h"

#include "msg.h"


static IpfixAggregator* buildAggregator(Rules* rules, uint16_t minBufferTime, uint16_t maxBufferTime);


int initializeAggregators()
{
	return 0;
}

int deinitializeAggregators()
{
	return 0;
}

/**
 * Creates a new Aggregator. Do not forget to set the callback functions, then call @c startAggregator().
 * @param ruleFile filename of file containing a set of rules
 * @param minBufferTime TODO
 * @param maxBufferTime TODO
 * @return handle to the Aggregator on success or NULL on error
 */

IpfixAggregator* createAggregator(char* ruleFile, uint16_t minBufferTime, uint16_t maxBufferTime)
{
	Rules* rules = parseRulesFromFile(ruleFile);
	if (!rules) {
		msg(MSG_FATAL, "Aggregator: could not parse rules file %s", ruleFile);
		return NULL;
	}
	return buildAggregator(rules, minBufferTime, maxBufferTime);
}

/**
 * Creates a new Aggregator. Do not forget to set the callback functions, then call @c startAggreagtor().
 * @param rules Rules for aggregator to work with
 * @param minBufferTime TODO
 * @param maxBufferTime TODO
 * @return handle to the Aggreagtor on success or NULL on error
 */
IpfixAggregator* createAggregatorFromRules(Rules* rules, uint16_t minBufferTime, uint16_t maxBufferTime)
{
	return buildAggregator(rules, minBufferTime, maxBufferTime);
}

/**
 * Builds a new aggregator from the given rules (helper function for @c createAggregator and @c createAggregatorFromRules)
 */
static IpfixAggregator* buildAggregator(Rules* rules, uint16_t minBufferTime, uint16_t maxBufferTime)
{
	int i;

	IpfixAggregator* ipfixAggregator = (IpfixAggregator*)malloc(sizeof(IpfixAggregator));
	ipfixAggregator->rules = rules;

	for (i = 0; i < rules->count; i++) {
		rules->rule[i]->hashtable = createHashtable(rules->rule[i],
							    minBufferTime,
							    maxBufferTime
							   );
	}

	if (pthread_mutex_init(&ipfixAggregator->mutex, NULL) != 0) {
		msg(MSG_FATAL, "Could not init mutex");
		}
		
	if (pthread_mutex_lock(&ipfixAggregator->mutex) != 0) {
		msg(MSG_FATAL, "Could not lock mutex");
		}

	msg(MSG_INFO, "Aggregator: Done. Parsed %d rules; minBufferTime %d, maxBufferTime %d",
	    rules->count, minBufferTime, maxBufferTime
	   );

        return ipfixAggregator;
}

/**
 * Frees memory used by an Aggregator.
 * Make sure the Aggregator is not being used before calling this method.
 * @param ipfixAggregator handle of Aggregator
 */
void destroyAggregator(IpfixAggregator* ipfixAggregator)
{
	Rules* rules = ((IpfixAggregator*)ipfixAggregator)->rules;

	int i;
	for (i = 0; i < rules->count; i++) {
		destroyHashtable(rules->rule[i]->hashtable);

	pthread_mutex_unlock(&((IpfixAggregator*)ipfixAggregator)->mutex);
	pthread_mutex_destroy(&((IpfixAggregator*)ipfixAggregator)->mutex);

	free(ipfixAggregator);
	}

	destroyRules(rules);
}

/**
 * Starts or resumes processing Records
 * @param ipfixAggregator handle of Aggregator
 */
void startAggregator(IpfixAggregator* ipfixAggregator)
{
	pthread_mutex_unlock(&ipfixAggregator->mutex);
}

/**
 * Temporarily pauses processing Records
 * @param ipfixAggregator handle of Aggregator
 */
void stopAggregator(IpfixAggregator* ipfixAggregator)
{
	pthread_mutex_lock(&ipfixAggregator->mutex);
}

/**
 * Injects new DataRecords into the Aggregator.
 * @param ipfixAggregator handle of aggregator this Record is for
 * @param sourceID ignored
 * @param ti structure describing @c data
 * @param length length (in bytes) of @c data
 * @param data raw data block containing the Record
 * @return 0 on success, non-zero on error
 */
int aggregateDataRecord(void* ipfixAggregator, SourceID sourceID, TemplateInfo* ti, uint16_t length, FieldData* data)
{
	Rules* rules = ((IpfixAggregator*)ipfixAggregator)->rules;

	int i;
	DPRINTF("aggregateDataRecord: Got a Data Record\n");

	if(!rules) {
		msg(MSG_FATAL, "Aggregator not started");
		return -1;
	}

	pthread_mutex_lock(&((IpfixAggregator*)ipfixAggregator)->mutex);
	for (i = 0; i < rules->count; i++) {
		if (templateDataMatchesRule(ti, data, rules->rule[i])) {
			DPRINTF("rule %d matches\n", i);
			aggregateTemplateData(rules->rule[i]->hashtable, ti, data);
		}
	}
	pthread_mutex_unlock(&((IpfixAggregator*)ipfixAggregator)->mutex);
	
	return 0;
}

/**
 * Injects new DataRecords into the Aggregator.
 * @param ipfixAggregator handle of aggregator this Record is for
 * @param sourceID ignored
 * @param ti structure describing @c data
 * @param length length (in bytes) of @c data
 * @param data raw data block containing the Record
 * @return 0 on success, non-zero on error
 */
int aggregateDataDataRecord(void* ipfixAggregator, SourceID sourceID, DataTemplateInfo* ti, uint16_t length, FieldData* data)
{
	Rules* rules = ((IpfixAggregator*)ipfixAggregator)->rules;

	int i;
	DPRINTF("aggregateDataDataRecord: Got a DataData Record\n");

	if(!rules) {
		msg(MSG_FATAL, "Aggregator not started");
		return -1;
	}

	pthread_mutex_lock(&((IpfixAggregator*)ipfixAggregator)->mutex);
	for (i = 0; i < rules->count; i++) {
		if (dataTemplateDataMatchesRule(ti, data, rules->rule[i])) {
			DPRINTF("rule %d matches\n", i);
			aggregateDataTemplateData(rules->rule[i]->hashtable, ti, data);
		}
	}
	pthread_mutex_unlock(&((IpfixAggregator*)ipfixAggregator)->mutex);
	
	return 0;
}

/**
 * Checks for flows buffered longer than @c ipfixAggregator::minBufferTime and/or @c ipfixAggregator::maxBufferTime and passes them to the previously defined callback functions.
 * @param ipfixAggregator handle of Aggregator to poll
 */
void pollAggregator(IpfixAggregator* ipfixAggregator)
{
        int i;
	Rules* rules = ((IpfixAggregator*)ipfixAggregator)->rules;

	pthread_mutex_lock(&ipfixAggregator->mutex);
	for (i = 0; i < rules->count; i++) {
		expireFlows(rules->rule[i]->hashtable);
	}
	pthread_mutex_unlock(&ipfixAggregator->mutex);
}

/**
 * Adds a set of callback functions to the list of functions to call when Templates or Records have to be sent
 * @param ipfixAggregator IpfixAggregator to set the callback function for
 * @param handles set of callback functions
 */
void addAggregatorCallbacks(IpfixAggregator* ipfixAggregator, CallbackInfo handles)
{
	int i;
	Rules* rules = ((IpfixAggregator*)ipfixAggregator)->rules;

	for (i = 0; i < rules->count; i++) {
		hashingAddCallbacks(rules->rule[i]->hashtable, handles);
	}
}

CallbackInfo getAggregatorCallbackInfo(IpfixAggregator* ipfixAggregator)
{
	CallbackInfo ci;

	bzero(&ci, sizeof(CallbackInfo));
	ci.handle = ipfixAggregator;
	ci.dataRecordCallbackFunction = aggregateDataRecord;
	ci.dataDataRecordCallbackFunction = aggregateDataDataRecord;

	return ci;
}

/**
 * Called by the logger timer thread. Dumps info using msg_stat
 */
void statsAggregator(void* ipfixAggregator_)
{
	int i;
	IpfixAggregator* ipfixAggregator = (IpfixAggregator*)ipfixAggregator_;
	Rules* rules = ipfixAggregator->rules;

	pthread_mutex_lock(&ipfixAggregator->mutex);
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
			HashBucket* hb = ht->bucket[j];
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
	pthread_mutex_unlock(&ipfixAggregator->mutex);
}
