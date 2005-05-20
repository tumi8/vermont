#ifndef AGGREGATOR_H
#define AGGREGATOR_H

#include "rcvIpfix.h"
#include "rules.h"
#include "hashing.h"

#ifdef __cplusplus
extern "C" {
#endif

/***** Constants ************************************************************/

/***** Typedefs *************************************************************/

/**
 * Represents an Aggregator.
 * Create with @c createAggregator()
 */
typedef struct {
        Rules* rules; /**< Set of rules that define the aggregator */
} IpfixAggregator;

/***** Prototypes ***********************************************************/

int initializeAggregators();
int deinitializeAggregators();

IpfixAggregator* createAggregator(char* ruleFile, uint16_t minBufferTime, uint16_t maxBufferTime);
void destroyAggregator(IpfixAggregator* ipfixAggregator);

void startAggregator(IpfixAggregator* ipfixAggregator);
void stopAggregator(IpfixAggregator* ipfixAggregator);

int aggregateDataRecord(void* ipfixAggregator, SourceID sourceID, TemplateInfo* ti, uint16_t length, FieldData* data);
int aggregateDataDataRecord(void* ipfixAggregator, SourceID sourceID, DataTemplateInfo* ti, uint16_t length, FieldData* data);

void pollAggregator(IpfixAggregator* ipfixAggregator);

void addAggregatorCallbacks(IpfixAggregator* ipfixAggregator, CallbackInfo handles);

CallbackInfo getAggregatorCallbackInfo(IpfixAggregator* ipfixAggregator);


#ifdef __cplusplus
}
#endif

#endif
