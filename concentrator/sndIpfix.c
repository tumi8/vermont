/** @file
 * IPFIX Exporter interface.
 *
 * Interface for feeding generated Templates and Data Records to "ipfixlolib"
 *
 */

#include <string.h>
#include "sndIpfix.h"
#include "ipfixlolib/ipfixlolib.h"
#include "ipfix.h"

#include "msg.h"

/***** Global Variables ******************************************************/

static uint8_t ringbufferPos = 0; /**< Pointer to next free slot in @c conversionRingbuffer. */
static uint8_t conversionRingbuffer[1 << (8 * sizeof(ringbufferPos))]; /**< Ringbuffer used to store converted imasks between @c ipfix_put_data_field() and @c ipfix_send() */

/***** Internal Functions ****************************************************/

/***** Exported Functions ****************************************************/

/**
 * Initializes internal structures.
 * To be called on application startup
 * @return 0 on success
 */
int initializeIpfixSenders() {
	return 0;
}

/**
 * Deinitializes internal structures.
 * To be called on application shutdown
 * @return 0 on success
 */
int deinitializeIpfixSenders() {
	return 0;
}

/**
 * Creates a new IPFIX Exporter. Do not forget to call @c startIpfixSender() to begin sending
 * @param sourceID Source ID this exporter will report
 * @param ip destination collector's address
 * @param port destination collector's port
 * @return handle to use when calling @c destroyIpfixSender()
 */
IpfixSender* createIpfixSender(SourceID sourceID, char* ip, uint16_t port) {
	IpfixSender* ipfixSender = (IpfixSender*)malloc(sizeof(IpfixSender));
	ipfix_exporter** exporterP = (ipfix_exporter**)&ipfixSender->ipfixExporter;
	strcpy(ipfixSender->ip, ip);
	ipfixSender->port = port;

	ipfixSender->lastTemplateId = 10000;
	if (ipfix_init_exporter(sourceID, exporterP) != 0) {
		msg(MSG_FATAL, "ipfix_init_exporter failed");
		return NULL;
	}

	if (ipfix_add_collector(*exporterP, ipfixSender->ip, ipfixSender->port, UDP) != 0) {
		msg(MSG_FATAL, "ipfix_add_collector failed");
		return NULL;
	}

        msg(MSG_DEBUG, "IPFIXSender: running");

	return ipfixSender;
}

/**
 * Removes a collector from the list of Collectors to send Records to
 * @param ipfixSender handle obtained by calling @c createIpfixSender()
 */
void destroyIpfixSender(IpfixSender* ipfixSender) {
	ipfix_exporter* exporter = (ipfix_exporter*)ipfixSender->ipfixExporter;

	if (ipfix_remove_collector(exporter, ipfixSender->ip, ipfixSender->port) != 0) {
		msg(MSG_FATAL, "ipfix_remove_collector failed");
	}
	ipfix_deinit_exporter(exporter);
	free(ipfixSender);
}

/**
 * Starts or resumes sending messages
 * @param ipfixSender handle to the Exporter
 */
void startIpfixSender(IpfixSender* ipfixSender) {
	/* unimplemented, we can't be paused - TODO: or should we? */
}

/**
 * Temporarily pauses sending messages
 * @param ipfixSender handle to the Exporter
 */
void stopIpfixSender(IpfixSender* ipfixSender) {
	/* unimplemented, we can't be paused - TODO: or should we? */
}

/**
 * Announces a new Template
 * @param ipfixSender_ handle to the Exporter
 * @param sourceID ignored
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
int sndNewDataTemplate(void* ipfixSender_, SourceID sourceID, DataTemplateInfo* dataTemplateInfo) {
	IpfixSender* ipfixSender = ipfixSender_;
	ipfix_exporter* exporter = (ipfix_exporter*)ipfixSender->ipfixExporter;
	if (!exporter) {
		msg(MSG_FATAL, "Exporter not set");
		return -1;
	}

	uint16_t my_template_id = ++ipfixSender->lastTemplateId;
	if (ipfixSender->lastTemplateId > 60000) {
		/* FIXME: Does not always work, e.g. if more than 50000 new Templates per minute are created */
		ipfixSender->lastTemplateId = 10000;
	}

	/* put Template ID in Template's userData */
	int* p = (int*)malloc(sizeof(int));
	*p = htons(my_template_id);
	dataTemplateInfo->userData = p;

	int i;

	/* Count number of IPv4 fields with length 5 */
	int splitFields = 0;
	for (i = 0; i < dataTemplateInfo->fieldCount; i++) {
		FieldInfo* fi = &dataTemplateInfo->fieldInfo[i];
		if ((fi->type.id == IPFIX_TYPEID_sourceIPv4Address) && (fi->type.length == 5)) {
			splitFields++;
		}
		else if ((fi->type.id == IPFIX_TYPEID_destinationIPv4Address) && (fi->type.length == 5)) {
			splitFields++;
		}
	}

	/* Count number of IPv4 fields with length 5 */
	int splitFixedfields = 0;
	for (i = 0; i < dataTemplateInfo->dataCount; i++) {
		FieldInfo* fi = &dataTemplateInfo->dataInfo[i];
		if ((fi->type.id == IPFIX_TYPEID_sourceIPv4Address) && (fi->type.length == 5)) {
			splitFixedfields++;
		}
		else if ((fi->type.id == IPFIX_TYPEID_destinationIPv4Address) && (fi->type.length == 5)) {
			splitFixedfields++;
		}
	}

	if (0 != ipfix_start_datatemplate_set(exporter, my_template_id, dataTemplateInfo->fieldCount + splitFields, dataTemplateInfo->dataCount + splitFixedfields)) {
		msg(MSG_FATAL, "ipfix_start_datatemplate_set failed");
		return -1;
	}

	for (i = 0; i < dataTemplateInfo->fieldCount; i++) {
		FieldInfo* fi = &dataTemplateInfo->fieldInfo[i];

		/* Split IPv4 fields with length 5, i.e. fields with network mask attached */
		if ((fi->type.id == IPFIX_TYPEID_sourceIPv4Address) && (fi->type.length == 5)) {
			ipfix_put_template_field(exporter, my_template_id, IPFIX_TYPEID_sourceIPv4Address, 4, 0);
			ipfix_put_template_field(exporter, my_template_id, IPFIX_TYPEID_sourceIPv4Mask, 1, 0);
		}
		else if ((fi->type.id == IPFIX_TYPEID_destinationIPv4Address) && (fi->type.length == 5)) {
			ipfix_put_template_field(exporter, my_template_id, IPFIX_TYPEID_destinationIPv4Address, 4, 0);
			ipfix_put_template_field(exporter, my_template_id, IPFIX_TYPEID_destinationIPv4Mask, 1, 0);
		}
		else {
			ipfix_put_template_field(exporter, my_template_id, fi->type.id, fi->type.length, fi->type.eid);
		}
	}

	DPRINTF("%d data fields\n", dataTemplateInfo->dataCount);

	int dataLength = 0;
	for (i = 0; i < dataTemplateInfo->dataCount; i++) {
		FieldInfo* fi = &dataTemplateInfo->dataInfo[i];

		dataLength += fi->type.length;

		/* Split IPv4 fields with length 5, i.e. fields with network mask attached */
		if ((fi->type.id == IPFIX_TYPEID_sourceIPv4Address) && (fi->type.length == 5)) {
			ipfix_put_template_fixedfield(exporter, my_template_id, IPFIX_TYPEID_sourceIPv4Address, 4, 0);
			ipfix_put_template_fixedfield(exporter, my_template_id, IPFIX_TYPEID_sourceIPv4Mask, 1, 0);
		}
		else if ((fi->type.id == IPFIX_TYPEID_destinationIPv4Address) && (fi->type.length == 5)) {
			ipfix_put_template_fixedfield(exporter, my_template_id, IPFIX_TYPEID_destinationIPv4Address, 4, 0);
			ipfix_put_template_fixedfield(exporter, my_template_id, IPFIX_TYPEID_destinationIPv4Mask, 1, 0);
		}
		else {
			ipfix_put_template_fixedfield(exporter, my_template_id, fi->type.id, fi->type.length, fi->type.eid);
		}
	}

	DPRINTF("%d data length\n", dataLength);

	char* data = (char*)malloc(dataLength);
	memcpy(data, dataTemplateInfo->data, dataLength);
	for (i = 0; i < dataTemplateInfo->fieldCount; i++) {
		FieldInfo* fi = &dataTemplateInfo->fieldInfo[i];

		/* Invert imask of IPv4 fields with length 5, i.e. fields with network mask attached */
		if ((fi->type.id == IPFIX_TYPEID_sourceIPv4Address) && (fi->type.length == 5)) {
			uint8_t* mask = (uint8_t*)(data + fi->offset + 4);
			*mask = 32 - *mask;
		}
		else if ((fi->type.id == IPFIX_TYPEID_destinationIPv4Address) && (fi->type.length == 5)) {
			uint8_t* mask = (uint8_t*)(data + fi->offset + 4);
			*mask = 32 - *mask;
		}
		else {
		}

	}

	if (0 != ipfix_put_template_data(exporter, my_template_id, data, dataLength)) {
		msg(MSG_FATAL, "ipfix_put_template_data failed");
		return -1;
	}
	free(data);

	if (0 != ipfix_end_template_set(exporter, my_template_id)) {
		msg(MSG_FATAL, "ipfix_end_template_set failed");
		return -1;
	}

	return 0;
}

/**
 * Invalidates a template; Does NOT free dataTemplateInfo
 * @param ipfixSender_ handle to the Exporter
 * @param sourceID ignored
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
int sndDestroyDataTemplate(void* ipfixSender_, SourceID sourceID, DataTemplateInfo* dataTemplateInfo) {
	free(dataTemplateInfo->userData);
	return 0;
}

/**
 * Put new Data Record in outbound exporter queue
 * @param ipfixSender_ handle to the Exporter
 * @param sourceID ignored
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 * @param length Length of the data block supplied
 * @param data Pointer to a data block containing all variable fields
 */
int sndDataDataRecord(void* ipfixSender_, SourceID sourceID, DataTemplateInfo* dataTemplateInfo, uint16_t length, FieldData* data) {
	IpfixSender* ipfixSender = ipfixSender_;
	ipfix_exporter* exporter = (ipfix_exporter*)ipfixSender->ipfixExporter;

	/* get Template ID from Template's userData */
	uint16_t my_n_template_id = *(uint16_t*)dataTemplateInfo->userData;

	if (ipfix_start_data_set(exporter, &my_n_template_id) != 0 ) {
		msg(MSG_FATAL, "ipfix_start_data_set failed!");
		return -1;
	}

	int i;
	for (i = 0; i < dataTemplateInfo->fieldCount; i++) {
		FieldInfo* fi = &dataTemplateInfo->fieldInfo[i];

		/* Split IPv4 fields with length 5, i.e. fields with network mask attached */
		if ((fi->type.id == IPFIX_TYPEID_sourceIPv4Address) && (fi->type.length == 5)) {
			uint8_t* mask = &conversionRingbuffer[ringbufferPos++];
			*mask = 32 - *(uint8_t*)(data + fi->offset + 4);
			ipfix_put_data_field(exporter, data + fi->offset, 4);
			ipfix_put_data_field(exporter, mask, 1);
		}
		else if ((fi->type.id == IPFIX_TYPEID_destinationIPv4Address) && (fi->type.length == 5)) {
			uint8_t* mask = &conversionRingbuffer[ringbufferPos++];
			*mask = 32 - *(uint8_t*)(data + fi->offset + 4);
			ipfix_put_data_field(exporter, data + fi->offset, 4);
			ipfix_put_data_field(exporter, mask, 1);
		}
		else {
			ipfix_put_data_field(exporter, data + fi->offset, fi->type.length);
		}

	}

	if (ipfix_end_data_set(exporter) != 0) {
		msg(MSG_FATAL, "ipfix_end_data_set failed");
		return -1;
	}

	if (ipfix_send(exporter) != 0) {
		msg(MSG_FATAL, "ipfix_send failed");
		return -1;
	}

	return 0;
}

CallbackInfo getIpfixSenderCallbackInfo(IpfixSender* ipfixSender) {
	CallbackInfo ci;
	bzero(&ci, sizeof(CallbackInfo));
	ci.handle = ipfixSender;
	ci.dataTemplateCallbackFunction = sndNewDataTemplate;
	ci.dataDataRecordCallbackFunction = sndDataDataRecord;
	ci.dataTemplateDestructionCallbackFunction = sndDestroyDataTemplate;
	return ci;
}
