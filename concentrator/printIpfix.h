#ifndef PRINTIPFIX_H
#define PRINTIPFIX_H

#include "rcvIpfix.h"

#ifdef __cplusplus
extern "C" {
#endif


/***** Constants ************************************************************/

/**
 * Represents an IpfixPrinter.
 * Create with @c createIpfixPrinter()
 */
typedef struct {
	int dummy;
} IpfixPrinter;

/***** Prototypes ***********************************************************/

int initializeIpfixPrinters();
int deinitializeIpfixPrinters();

IpfixPrinter* createIpfixPrinter();
void destroyIpfixPrinter(IpfixPrinter* ipfixPrinter);

void startIpfixPrinter(IpfixPrinter* ipfixPrinter);
void stopIpfixPrinter(IpfixPrinter* ipfixPrinter);

int printDataTemplate(void* ipfixPrinter, SourceID* sourceID, DataTemplateInfo* dataTemplateInfo);
int printDataDataRecord(void* ipfixPrinter, SourceID* sourceID, DataTemplateInfo* dataTemplateInfo, uint16_t length, FieldData* data);
int printDataTemplateDestruction(void* ipfixPrinter, SourceID* sourceID, DataTemplateInfo* dataTemplateInfo);

int printOptionsTemplate(void* ipfixPrinter, SourceID* sourceID, OptionsTemplateInfo* optionsTemplateInfo);
int printOptionsRecord(void* ipfixPrinter, SourceID* sourceID, OptionsTemplateInfo* optionsTemplateInfo, uint16_t length, FieldData* data);
int printOptionsTemplateDestruction(void* ipfixPrinter, SourceID* sourceID, OptionsTemplateInfo* optionsTemplateInfo);

int printTemplate(void* ipfixPrinter, SourceID* sourceID, TemplateInfo* templateInfo);
int printDataRecord(void* ipfixPrinter, SourceID* sourceID, TemplateInfo* templateInfo, uint16_t length, FieldData* data);
int printTemplateDestruction(void* ipfixPrinter, SourceID* sourceID, TemplateInfo* templateInfo);

CallbackInfo getIpfixPrinterCallbackInfo(IpfixPrinter* ipfixPrinter);

#ifdef __cplusplus
}
#endif

#endif
