#ifndef RCVIPFIX_H
#define RCVIPFIX_H

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

/**
 * IPFIX field type and length.
 * if "id" is < 0x8000, i.e. no user-defined type, "eid" is 0
 */ 
typedef struct {
	TypeId id;          /**< type tag of this field, according to [INFO] */
	FieldLength length; /**< length in bytes of this field */	
	EnterpriseNo eid;   /**< enterpriseNo for user-defined data types (i.e. type >= 0x8000) */	
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
	uint16_t   fieldCount;    /**< number of regular fields */
	FieldInfo* fieldInfo;     /**< array of FieldInfos describing each of these fields */
	void*      userData;      /**< pointer to a field that can be used by higher-level modules */
	} TemplateInfo;

/**
 * OptionsTemplate description passed to the callback function when a new OptionsTemplate arrives.
 * Note that - other than in [PROTO] - fieldCount specifies only the number of regular fields
 */
typedef struct {
	uint16_t   scopeCount;  /**< number of scope fields */
	FieldInfo* scopeInfo;   /**< array of FieldInfos describing each of these fields */
	uint16_t   fieldCount;  /**< number of regular fields. This is NOT the number of all fields */
	FieldInfo* fieldInfo;   /**< array of FieldInfos describing each of these fields */
	void*      userData;    /**< pointer to a field that can be used by higher-level modules */
	} OptionsTemplateInfo;

/**
 * DataTemplate description passed to the callback function when a new DataTemplate arrives.
 */
typedef struct {
	uint16_t   id;
	uint16_t   preceding;
	uint16_t   fieldCount;  /**< number of regular fields */
	FieldInfo* fieldInfo;   /**< array of FieldInfos describing each of these fields */
	uint16_t   dataCount;   /**< number of fixed-value fields */
	FieldInfo* dataInfo;    /**< array of FieldInfos describing each of these fields */
	FieldData* data;        /**< data start pointer for fixed-value fields */
	void*      userData;    /**< pointer to a field that can be used by higher-level modules */
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
 * Represents a Collector.
 * Create with @c createIpfixReceiver()
 */
typedef struct {
	int socket;
	pthread_mutex_t mutex;      /**< Mutex to pause receiving thread */
	pthread_t thread;	    /**< Thread ID for this particular instance, to sync against etc */
	
	int callbackCount;          /**< Length of callbackInfo array */
	CallbackInfo* callbackInfo; /**< Array of callback functions to invoke when new messages arrive */
	
	void* templateBuffer;       /**< TemplateBuffer* structure */

        int exit; /**< exit flag to terminate thread */
} IpfixReceiver;

/***** Prototypes ***********************************************************/

int initializeIpfixReceivers();
int deinitializeIpfixReceivers();

void printFieldData(FieldType type, FieldData* pattern);

FieldInfo* getTemplateFieldInfo(TemplateInfo* ti, FieldType* type);
FieldInfo* getDataTemplateFieldInfo(DataTemplateInfo* ti, FieldType* type);
FieldInfo* getDataTemplateDataInfo(DataTemplateInfo* ti, FieldType* type);

IpfixReceiver* createIpfixReceiver(uint16_t port);
void destroyIpfixReceiver(IpfixReceiver* ipfixReceiver);

int startIpfixReceiver(IpfixReceiver *ipr);
int stopIpfixReceiver(IpfixReceiver *ipr);
int shutdownIpfixReceiver(IpfixReceiver *ipr);

void addIpfixReceiverCallbacks(IpfixReceiver* ipfixReceiver, CallbackInfo handles);


#ifdef __cplusplus
}
#endif

#endif
