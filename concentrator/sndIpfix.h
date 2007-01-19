#ifndef SNDIPFIX_H
#define SNDIPFIX_H

#include "rcvIpfix.h"
#include "ipfixlolib/ipfixlolib.h"

/***** Constants ************************************************************/

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Represents an Exporter.
 * Create with @c createIpfixSender()
 */
typedef struct {
	ipfix_exporter* ipfixExporter; /**< underlying ipfix_exporter structure. Cast from void* to minimize header dependencies */
	uint16_t lastTemplateId; /**< Template ID of last created Template */
	char ip[128]; /**< IP of Collector we export to */
	uint16_t port; /**< Port of Collector we export to */
	uint32_t sentRecords; /**< Statistics: Total number of records sent since last statistics were polled */
} IpfixSender;

/***** Prototypes ***********************************************************/

int initializeIpfixSenders();
int deinitializeIpfixSenders();

IpfixSender* createIpfixSender(uint16_t observationDomainId, const char* ip, uint16_t port);
void destroyIpfixSender(IpfixSender* ipfixSender);

void startIpfixSender(IpfixSender* ipfixSender);
void stopIpfixSender(IpfixSender* ipfixSender);

int ipfixSenderAddCollector(IpfixSender *ips, const char *ip, uint16_t port);

int sndNewDataTemplate(void* ipfixSender, SourceID* sourceID, DataTemplateInfo* dataTemplateInfo);
int sndDestroyDataTemplate(void* ipfixSender, SourceID* sourceID, DataTemplateInfo* dataTemplateInfo);
int sndDataDataRecord(void* ipfixSender, SourceID* sourceID, DataTemplateInfo* dataTemplateInfo, uint16_t length, FieldData* data);

CallbackInfo getIpfixSenderCallbackInfo(IpfixSender* ipfixSender);

void statsIpfixSender(void* ipfixSender);

#ifdef __cplusplus
}
#endif

#endif

