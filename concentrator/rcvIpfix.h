#ifndef RCVIPFIX_H
#define RCVIPFIX_H

#include "ipfixReceiver.h"

#include <pthread.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/***** Constants ************************************************************/


/***** Data Types ***********************************************************/

typedef uint32_t SourceID;
typedef uint16_t TemplateID;
typedef uint16_t TypeId;
typedef uint16_t FieldLength;
typedef uint32_t EnterpriseNo;
typedef uint8_t FieldData;
typedef uint8_t byte;

/**
 * IPFIX field type and length.
 * if "id" is < 0x8000, i.e. no user-defined type, "eid" is 0
 */ 
typedef struct {
	TypeId id;            /**< type tag of this field, according to [INFO] */
	FieldLength length;   /**< length in bytes of this field */
	int isVariableLength; /**< true if this field's length might change from record to record, false otherwise */
	EnterpriseNo eid;     /**< enterpriseNo for user-defined data types (i.e. type >= 0x8000) */	
} FieldType;

/**
 * Information describing a single field in the fields passed via various callback functions.
 */
typedef struct {
	FieldType type;
	uint16_t offset;          /**< offset in bytes from a data start pointer. For internal purposes 65535 is defined as yet unknown */
} FieldInfo;

/**
 * Template description passed to the callback function when a new Template arrives.
 */
typedef struct {
	uint16_t   templateId;    /**< the template id assigned to this template or 0 if we don't know or don't care */
	uint16_t   fieldCount;    /**< number of regular fields */
	FieldInfo* fieldInfo;     /**< array of FieldInfos describing each of these fields */
	void*      userData;      /**< pointer to a field that can be used by higher-level modules */
} TemplateInfo;

/**
 * OptionsTemplate description passed to the callback function when a new OptionsTemplate arrives.
 * Note that - other than in [PROTO] - fieldCount specifies only the number of regular fields
 */
typedef struct {
	uint16_t   templateId;    /**< the template id assigned to this template or 0 if we don't know or don't care */
	uint16_t   scopeCount;    /**< number of scope fields */
	FieldInfo* scopeInfo;     /**< array of FieldInfos describing each of these fields */
	uint16_t   fieldCount;    /**< number of regular fields. This is NOT the number of all fields */
	FieldInfo* fieldInfo;     /**< array of FieldInfos describing each of these fields */
	void*      userData;      /**< pointer to a field that can be used by higher-level modules */
} OptionsTemplateInfo;

/**
 * DataTemplate description passed to the callback function when a new DataTemplate arrives.
 */
typedef struct {
	uint16_t   id;            /**< the template id assigned to this template or 0 if we don't know or don't care */
	uint16_t   preceding;     /**< the preceding rule field as defined in the draft */
	uint16_t   fieldCount;    /**< number of regular fields */
	FieldInfo* fieldInfo;     /**< array of FieldInfos describing each of these fields */
	uint16_t   dataCount;     /**< number of fixed-value fields */
	FieldInfo* dataInfo;      /**< array of FieldInfos describing each of these fields */
	FieldData* data;          /**< data start pointer for fixed-value fields */
	void*      userData;      /**< pointer to a field that can be used by higher-level modules */
} DataTemplateInfo;

/*** Template Callbacks ***/

/**
 * Callback function invoked when a new Template arrives.
 * @param handle handle passed to the callback function to differentiate different instances and/or operation modes
 * @param sourceId SourceID of the exporter that sent this Template
 * @param templateInfo Pointer to a structure defining this Template
 * @return 0 if packet handled successfully
 */
typedef int(TemplateCallbackFunction)(void* handle, SourceID sourceID, TemplateInfo* templateInfo);

/**
 * Callback function invoked when a new OptionsTemplate arrives.
 * @param handle handle passed to the callback function to differentiate different instances and/or operation modes
 * @param sourceId SourceID of the exporter that sent this OptionsTemplate
 * @param optionsTemplateInfo Pointer to a structure defining this OptionsTemplate
 * @return 0 if packet handled successfully
 */
typedef int(OptionsTemplateCallbackFunction)(void* handle, SourceID sourceID, OptionsTemplateInfo* optionsTemplateInfo);

/**
 * Callback function invoked when a new DataTemplate arrives.
 * @param handle handle passed to the callback function to differentiate different instances and/or operation modes
 * @param sourceId SourceID of the exporter that sent this DataTemplate
 * @return 0 if packet handled successfully
 */
typedef int(DataTemplateCallbackFunction)(void* handle, SourceID sourceID, DataTemplateInfo* dataTemplateInfo);

/*** Template Destruction Callbacks ***/
         
/**
 * Callback function invoked when a Template is being destroyed.
 * Particularly useful for cleaning up userData associated with this Template
 * @param handle handle passed to the callback function to differentiate different instances and/or operation modes
 * @param sourceId SourceID of the exporter that sent this Template
 * @param templateInfo Pointer to a structure defining this Template
 * @return 0 if packet handled successfully
 */
typedef int(TemplateDestructionCallbackFunction)(void* handle, SourceID sourceID, TemplateInfo* templateInfo);

/**
 * Callback function invoked when a OptionsTemplate is being destroyed.
 * Particularly useful for cleaning up userData associated with this Template
 * @param handle handle passed to the callback function to differentiate different instances and/or operation modes
 * @param sourceId SourceID of the exporter that sent this OptionsTemplate
 * @param optionsTemplateInfo Pointer to a structure defining this OptionsTemplate
 * @return 0 if packet handled successfully
 */
typedef int(OptionsTemplateDestructionCallbackFunction)(void* handle, SourceID sourceID, OptionsTemplateInfo* optionsTemplateInfo);

/**
 * Callback function invoked when a DataTemplate is being destroyed.
 * Particularly useful for cleaning up userData associated with this Template
 * @param handle handle passed to the callback function to differentiate different instances and/or operation modes
 * @param sourceId SourceID of the exporter that sent this DataTemplate
 * @return 0 if packet handled successfully
 */
typedef int(DataTemplateDestructionCallbackFunction)(void* handle, SourceID sourceID, DataTemplateInfo* dataTemplateInfo);

/*** Data Callbacks ***/

/**
 * Callback function invoked when a new Data Record arrives.
 * @param handle handle passed to the callback function to differentiate different instances and/or operation modes
 * @param sourceId SourceID of the exporter that sent this Record
 * @param templateInfo Pointer to a structure defining the Template used
 * @param length Length of the data block supplied
 * @param data Pointer to a data block containing all fields
 * @return 0 if packet handled successfully
 */
typedef int(DataRecordCallbackFunction)(void* handle, SourceID sourceID, TemplateInfo* templateInfo, uint16_t length, FieldData* data);

/**
 * Callback function invoked when a new Options Record arrives.
 * @param handle handle passed to the callback function to differentiate different instances and/or operation modes
 * @param sourceId SourceID of the exporter that sent this Record
 * @param optionsTemplateInfo Pointer to a structure defining the OptionsTemplate used
 * @param length Length of the data block supplied
 * @param data Pointer to a data block containing all fields
 * @return 0 if packet handled successfully
 */
typedef int(OptionsRecordCallbackFunction)(void* handle, SourceID sourceID, OptionsTemplateInfo* optionsTemplateInfo, uint16_t length, FieldData* data);

/**
 * Callback function invoked when a new Data Record with associated Fixed Values arrives.
 * @param handle handle passed to the callback function to differentiate different instances and/or operation modes
 * @param sourceId SourceID of the exporter that sent this Record
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 * @param length Length of the data block supplied
 * @param data Pointer to a data block containing all variable fields
 * @return 0 if packet handled successfully
 */
typedef int(DataDataRecordCallbackFunction)(void* handle, SourceID sourceID, DataTemplateInfo* dataTemplateInfo, uint16_t length, FieldData* data);

/**
 * Collection of callback functions used for passing Templates and Data Records between modules.
 */
typedef struct {
	void* handle; /**< handle passed to the callback functions to differentiate different instances and/or operation modes */

	TemplateCallbackFunction* templateCallbackFunction;
	OptionsTemplateCallbackFunction* optionsTemplateCallbackFunction;
	DataTemplateCallbackFunction* dataTemplateCallbackFunction;

	DataRecordCallbackFunction* dataRecordCallbackFunction;
	OptionsRecordCallbackFunction* optionsRecordCallbackFunction;
	DataDataRecordCallbackFunction* dataDataRecordCallbackFunction;

	TemplateDestructionCallbackFunction* templateDestructionCallbackFunction;
	OptionsTemplateDestructionCallbackFunction* optionsTemplateDestructionCallbackFunction;
	DataTemplateDestructionCallbackFunction* dataTemplateDestructionCallbackFunction;
} CallbackInfo;


/**
 * Contains information about parsing process
 * created by @c createIpfixParser()
 */
typedef struct {
	int callbackCount;          /**< Length of callbackInfo array */
	CallbackInfo* callbackInfo; /**< Array of callback functions to invoke when new messages arrive */

	void* templateBuffer;       /**< TemplateBuffer* structure */
} IpfixParser;

        
/**
 * Callback function invoked when a new packet arrives.
 * @param ipfixParser parser containing callbackfunction invoked while parsing message.
 * @param message Raw message data
 * @param len Length of message
 */
typedef int(ProcessPacketCallbackFunction)(IpfixParser* ipfixParser, byte* message, uint16_t len);

/**
 * Controls parsing of incoming packets.
 * Create witch @c createPacketProcessor()
 */
typedef struct {
	ProcessPacketCallbackFunction* processPacketCallbackFunction; /**< Callback function invoked when new packet arrives. */
	IpfixParser* ipfixParser; /**< Contains information about parsing process */

	pthread_mutex_t mutex; /**< Used to give only one IpfixReceiver access to the IpfixPacketProcessor */
} IpfixPacketProcessor;


/**
 * Represents a collector
 */
typedef struct {
	int receiverCount;
	IpfixReceiver** ipfixReceivers;

	int processorCount;
	IpfixPacketProcessor* packetProcessors;
} IpfixCollector;

/***** Prototypes ***********************************************************/


/* ------------------------------- Collector && Collector-Stuff --------------------------------- */

int initializeIpfixCollectors();
int deinitializeIpfixCollectors();
IpfixCollector* createIpfixCollector();
void destroyIpfixCollector(IpfixCollector* ipfixCollector);
int startIpfixCollector(IpfixCollector*);
int stopIpfixCollector(IpfixCollector*);

/* ---------------------------------------------- Processor --------------------------------------- */

IpfixPacketProcessor* createIpfixPacketProcessor();
void destroyIpfixPacketProcessor(IpfixPacketProcessor* packetProcessor);

/* --------------------------------------- Parser && Parsing Stuff  ------------------------------- */

IpfixParser* createIpfixParser();
void destroyIpfixParser(IpfixParser* ipfixParser);


void printFieldData(FieldType type, FieldData* pattern);

FieldInfo* getTemplateFieldInfo(TemplateInfo* ti, FieldType* type);
FieldInfo* getDataTemplateFieldInfo(DataTemplateInfo* ti, FieldType* type);
FieldInfo* getDataTemplateDataInfo(DataTemplateInfo* ti, FieldType* type);

/* --------------------------------------- Connectors --------------------------------------------- */

void addIpfixParserCallbacks(IpfixParser* ipfixParser, CallbackInfo handles);
void setIpfixParser(IpfixPacketProcessor* packetProcessor, IpfixParser* ipfixParser);

void addIpfixPacketProcessor(IpfixCollector* ipfixCollector, IpfixPacketProcessor* packetProcessor);
void addIpfixReceiver(IpfixCollector* ipfixCollector, IpfixReceiver* ipfixReceiver);

#ifdef __cplusplus
}
#endif


#endif
