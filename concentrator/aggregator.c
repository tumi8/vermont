/** \file
 * Aggregator module.
 * TODO: add long description
 */

#include <netinet/in.h>
#include <unistd.h>
#include "aggregator.h"
#include "rcvIpfix.h"
#include "sndIpfix.h"

#include "msg.h"

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
	int i;

	IpfixAggregator* ipfixAggregator = (IpfixAggregator*)malloc(sizeof(IpfixAggregator));
	ipfixAggregator->rules = parseRulesFromFile(ruleFile);

	if (!ipfixAggregator->rules) {
		msg(MSG_FATAL, "Aggregator: could not parse rules file %s", ruleFile);
		return NULL;
	}

	Rules* rules = ipfixAggregator->rules;
	for (i = 0; i < rules->count; i++) {
		rules->rule[i]->hashtable = createHashtable(rules->rule[i],
							    minBufferTime,
							    maxBufferTime
							   );
	}

	msg(MSG_INFO, "Aggregator: parsed %d rules; minBufferTime %d, maxBufferTime %d",
	    i, minBufferTime, maxBufferTime
	   );

	return ipfixAggregator;
}

/**
 * Frees memory used by an Aggregator
 * @param ipfixAggregator handle of Aggregator
 */
void destroyAggregator(IpfixAggregator* ipfixAggregator)
{
	Rules* rules = ((IpfixAggregator*)ipfixAggregator)->rules;

	int i;
	for (i = 0; i < rules->count; i++) {
		destroyHashtable(rules->rule[i]->hashtable);
	}

	destroyRules(rules);
}

/**
 * Starts or resumes processing Records
 * @param ipfixAggregator handle of Aggregator
 */
void startAggregator(IpfixAggregator* ipfixAggregator)
{
	//FIXME: unimplemented
}

/**
 * Temporarily pauses processing Records
 * @param ipfixAggregator handle of Aggregator
 */
void stopAggregator(IpfixAggregator* ipfixAggregator)
{
	//FIXME: unimplemented
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

	for (i = 0; i < rules->count; i++) {
		if (templateDataMatchesRule(ti, data, rules->rule[i])) {
			DPRINTF("rule %d matches\n", i);
			aggregateTemplateData(rules->rule[i]->hashtable, ti, data);
		}
	}
	
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

	for (i = 0; i < rules->count; i++) {
		if (dataTemplateDataMatchesRule(ti, data, rules->rule[i])) {
			DPRINTF("rule %d matches\n", i);
			aggregateDataTemplateData(rules->rule[i]->hashtable, ti, data);
		}
	}

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

	for (i = 0; i < rules->count; i++) {
		expireFlows(rules->rule[i]->hashtable);
	}
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
