#ifndef HASHING_H
#define HASHING_H

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "rcvIpfix.h"
#include "rules.h"

/***** Constants ************************************************************/

#define HASHTABLE_SIZE	65536

/***** Data Types ***********************************************************/

/**
 * Single Bucket containing one buffered flow's variable data.
 * Is either a direct entry in @c Hashtable::bucket or a member of another HashBucket's spillchain
 */
typedef struct {
	uint32_t expireTime;      /**< timestamp when this bucket will expire if no new flows are added */
	uint32_t forceExpireTime; /**< timestamp when this bucket is forced to expire */
	FieldData* data;        /**< contains variable fields of aggregated flow; format defined in Hashtable::dataInfo::fieldInfo */
	void* next;             /**< next bucket in spill chain */
} HashBucket;

/**
 * Hash-powered buffer for outgoing flows.
 * This is where outbound flows are aggregated while waiting to be exported.
 */	
typedef struct {
	int bucketCount;                     /**< size of this hashtable (must be HASHTABLE_SIZE) */
	HashBucket* bucket[HASHTABLE_SIZE];  /**< array of pointers to hash buckets at start of spill chain. Members are NULL where no entry present */
	
	DataTemplateInfo* dataTemplate;      /**< structure describing both variable and fixed fields and containing fixed data */
	uint16_t fieldLength;                /**< length in bytes of all variable-length fields */
	FieldModifier* fieldModifier;        /**< specifies what modifier to apply to a given field */

	uint16_t minBufferTime;              /**< If for a buffered flow no new aggregatable flows arrive for this many seconds, export it */
	uint16_t maxBufferTime;              /**< If a buffered flow was kept buffered for this many seconds, export it */

	int callbackCount;                   /**< Length of callbackInfo array */
	CallbackInfo* callbackInfo;          /**< Array of callback functions to invoke when Templates or Records should be sent */
} Hashtable;
	
/***** Prototypes ***********************************************************/

Hashtable* createHashtable(Rule* rule, uint16_t minBufferTime, uint16_t maxBufferTime);
void destroyHashtable(Hashtable* ht);

void hashingAddCallbacks(Hashtable* ht, CallbackInfo handles);

void aggregateTemplateData(Hashtable* ht, TemplateInfo* ti, FieldData* data);
void aggregateDataTemplateData(Hashtable* ht, DataTemplateInfo* ti, FieldData* data);

void destroyHashtable(Hashtable* ht);

void expireFlows(Hashtable* ht);

#endif
