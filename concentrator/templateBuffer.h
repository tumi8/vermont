#ifndef TEMPLATEBUFFER_H
#define TEMPLATEBUFFER_H

#include "rcvIpfix.h"
#include "time.h"

#ifdef __cplusplus
extern "C" {
#endif


/***** Constants ************************************************************/

#define TEMPLATE_EXPIRE_SECS  60

/***** Data Types ************************************************************/

/**
 * Represents a single Buffered Template
 */
typedef struct {
	SourceID	sourceID;     /**< source identifier of exporter that sent this template */
	TemplateID	templateID;   /**< template# this template defines */
	uint16_t	recordLength; /**< length of one Data Record that will be transferred in Data Sets. Variable-length carry -1 */
	TemplateID	setID;        /**< should be 2,3,4 and determines the type of pointer used in the unions */
	time_t		expires;      /**< Timestamp when this Template will expire or 0 if it will never expire */
	union {
		TemplateInfo* templateInfo;
		OptionsTemplateInfo* optionsTemplateInfo;
		DataTemplateInfo* dataTemplateInfo;
	};
	void*	next;             /**< Pointer to next buffered Template */
} BufferedTemplate;

/**
 * Represents a Template Buffer
 */
typedef struct {
	BufferedTemplate* head; /**< Start of BufferedTemplate chain */
	IpfixReceiver* ipfixReceiver; /**< Pointer to the ipfixReceiver which instantiated this TemplateBuffer */
} TemplateBuffer;


/***** Prototypes ************************************************************/

TemplateBuffer* createTemplateBuffer(IpfixReceiver* parentIpfixReceiver);
void destroyTemplateBuffer(TemplateBuffer* templateBuffer);
BufferedTemplate* getBufferedTemplate(TemplateBuffer* templateBuffer, SourceID sourceId, TemplateID templateId);
void destroyBufferedTemplate(TemplateBuffer* templateBuffer, SourceID sourceId, TemplateID id);
void bufferTemplate(TemplateBuffer* templateBuffer, BufferedTemplate* bt);


#ifdef __cplusplus
extern "C" {
#endif


#endif
