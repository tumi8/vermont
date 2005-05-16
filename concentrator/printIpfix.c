/** @file
 * IPFIX Exporter interface.
 *
 * Interface for feeding generated Templates and Data Records to "ipfixlolib"
 *
 */

#include <stdlib.h>
#include <string.h>
#include "printIpfix.h"

/***** Global Variables ******************************************************/

/***** Internal Functions ****************************************************/

/***** Exported Functions ****************************************************/

/**
 * Initializes internal structures.
 * To be called on application startup
 * @return 0 on success
 */
int initializeIpfixPrinters() {
	return 0;
}

/**
 * Deinitializes internal structures.
 * To be called on application shutdown
 * @return 0 on success
 */
int deinitializeIpfixPrinters() {
	return 0;
}

/**
 * Creates a new IpfixPrinter. Do not forget to call @c startIpfixPrinter() to begin printing
 * @return handle to use when calling @c destroyIpfixPrinter()
 */
IpfixPrinter* createIpfixPrinter() {
	IpfixPrinter* ipfixPrinter = (IpfixPrinter*)malloc(sizeof(IpfixPrinter));

	return ipfixPrinter;
}

/**
 * Frees memory used by an IpfixPrinter
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 */
void destroyIpfixPrinter(IpfixPrinter* ipfixPrinter) {
	free(ipfixPrinter);
}

/**
 * Starts or resumes printing messages
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 */
void startIpfixPrinter(IpfixPrinter* ipfixPrinter) {
	/* unimplemented, we can't be paused - TODO: or should we? */
}

/**
 * Temporarily pauses printing messages
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 */
void stopIpfixPrinter(IpfixPrinter* ipfixPrinter) {
	/* unimplemented, we can't be paused - TODO: or should we? */
}

/**
 * Prints a Template
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
int printTemplate(void* ipfixPrinter, SourceID sourceID, TemplateInfo* templateInfo) {
	printf("\n-+--- Template\n");
	printf(" `---\n\n");

	return 0;
}

/**
 * Prints a Template that was announced to be destroyed
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
int printTemplateDestruction(void* ipfixPrinter, SourceID sourceID, TemplateInfo* templateInfo) {
	printf("Destroyed a Template\n");

	return 0;
}

/**
 * Prints a DataRecord
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 * @param length Length of the data block supplied
 * @param data Pointer to a data block containing all variable fields
 */
int printDataRecord(void* ipfixPrinter, SourceID sourceID, TemplateInfo* templateInfo, uint16_t length, FieldData* data) {
	int i;

	printf("\n-+--- DataRecord\n");
	printf(" `- variable data\n");
	for (i = 0; i < templateInfo->fieldCount; i++) {
		printf(" '   `- ");
		printFieldData(templateInfo->fieldInfo[i].type, (data + templateInfo->fieldInfo[i].offset));
		printf("\n");
	}
	printf(" `---\n\n");

	return 0;
}

/**
 * Prints a OptionsTemplate
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
int printOptionsTemplate(void* ipfixPrinter, SourceID sourceID, OptionsTemplateInfo* optionsTemplateInfo) {

	printf("\n-+--- OptionsTemplate\n");
	printf(" `---\n\n");

	return 0;
}

/**
 * Prints a DataTemplate that was announced to be destroyed
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
int printOptionsTemplateDestruction(void* ipfixPrinter_, SourceID sourceID, OptionsTemplateInfo* optionsTemplateInfo) {
	printf("Destroyed an OptionsTemplate\n");

	return 0;
}

/**
 * Prints an OptionsRecord
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 * @param length Length of the data block supplied
 * @param data Pointer to a data block containing all variable fields
 */
int printOptionsRecord(void* ipfixPrinter, SourceID sourceID, OptionsTemplateInfo* optionsTemplateInfo, uint16_t length, FieldData* data) {

	printf("\n-+--- OptionsDataRecord\n");
	printf(" `---\n\n");

	return 0;
}

/**
 * Prints a DataTemplate
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
int printDataTemplate(void* ipfixPrinter, SourceID sourceID, DataTemplateInfo* dataTemplateInfo) {
	int i;

	printf("\n-+--- DataTemplate\n");
	printf(" `- fixed data\n");
	for (i = 0; i < dataTemplateInfo->dataCount; i++) {
		printf(" '   `- ");
		printFieldData(dataTemplateInfo->dataInfo[i].type, (dataTemplateInfo->data + dataTemplateInfo->dataInfo[i].offset));
		printf("\n");
	}
	printf(" `---\n\n");

	return 0;
}

/**
 * Prints a DataTemplate that was announced to be destroyed
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
int printDataTemplateDestruction(void* ipfixPrinter_, SourceID sourceID, DataTemplateInfo* dataTemplateInfo) {
	printf("Destroyed a DataTemplate\n");

	return 0;
}

/**
 * Prints a DataDataRecord
 * @param ipfixPrinter handle obtained by calling @c createIpfixPrinter()
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 * @param length Length of the data block supplied
 * @param data Pointer to a data block containing all variable fields
 */
int printDataDataRecord(void* ipfixPrinter, SourceID sourceID, DataTemplateInfo* dataTemplateInfo, uint16_t length, FieldData* data) {
	int i;

	printf("\n-+--- DataDataRecord\n");
	printf(" `- fixed data\n");
	for (i = 0; i < dataTemplateInfo->dataCount; i++) {
		printf(" '   `- ");
		printFieldData(dataTemplateInfo->dataInfo[i].type, (dataTemplateInfo->data + dataTemplateInfo->dataInfo[i].offset));
		printf("\n");
	}
	printf(" `- variable data\n");
	for (i = 0; i < dataTemplateInfo->fieldCount; i++) {
		printf(" '   `- ");
		printFieldData(dataTemplateInfo->fieldInfo[i].type, (data + dataTemplateInfo->fieldInfo[i].offset));
		printf("\n");
	}
	printf(" `---\n\n");

	return 0;
}

CallbackInfo getIpfixPrinterCallbackInfo(IpfixPrinter* ipfixPrinter) {
	CallbackInfo ci;
	bzero(&ci, sizeof(CallbackInfo));
	ci.handle = ipfixPrinter;
	ci.templateCallbackFunction = printTemplate;
	ci.dataRecordCallbackFunction = printDataRecord;
	ci.templateDestructionCallbackFunction = printTemplateDestruction;
	ci.optionsTemplateCallbackFunction = printOptionsTemplate;
	ci.optionsRecordCallbackFunction = printOptionsRecord;
	ci.optionsTemplateDestructionCallbackFunction = printOptionsTemplateDestruction;
	ci.dataTemplateCallbackFunction = printDataTemplate;
	ci.dataDataRecordCallbackFunction = printDataDataRecord;
	ci.dataTemplateDestructionCallbackFunction = printDataTemplateDestruction;
	return ci;
}
