/*
 this is vermont.
 released under GPL v2

 (C) by Ronny T. Lampert
 */

#define HOOK_SOURCE_ID 4913
/* just for abs */
#include <stdlib.h>

#include "aggregator.h"
#include "rcvIpfix.h"

#include "sampler/packet_hook.h"
#include "sampler_hook_entry.h"

#include "ipfix.h"
#include "printIpfix.h"

#include "msg.h"

#ifdef __cplusplus
extern "C" {
#endif

static FieldInfo ip_traffic_fi[] = {
	/*
	 { { ID, len, enterprise}, offset} for:
	 packet count
	 flowcreationtime
         flowendtime
	 length
	 proto
	 srcip
	 dstip
	 */
	{ {2, 1, 0}, 10},
	{ {22, 4, 0}, 4},
	{ {21, 4, 0}, 4},
	{ {1, 2, 0}, 2},
	{ {4, 1, 0}, 9},
	{ {8, 4, 0}, 12},
	{ {12, 4, 0}, 16}
};

static FieldInfo icmp_traffic_fi[] = {
	/*
	 { { ID, len, enterprise}, offset} for:
	 packet count
	 flowcreationtime
         flowendtime
	 length
	 proto
	 srcip
	 dstip
	 */
	{ {32, 2, 0}, 0},
	{ {2, 1, 0}, 10},
	{ {22, 4, 0}, 4},
	{ {21, 4, 0}, 4},
	{ {1, 2, 0}, 2},
	{ {4, 1, 0}, 9},
	{ {8, 4, 0}, 12},
	{ {12, 4, 0}, 16}
};

static FieldInfo tcpudp_traffic_fi[] = {
	/*
	 { { ID, len, enterprise}, offset} for:
	 srcport
	 dstport
	 packet count
	 flowcreationtime
         flowendtime
	 length
	 proto
	 srcip
	 dstip
	 */
	{ {7, 2, 0}, 0},
	{ {11, 2, 0}, 2},
	{ {2, 1, 0}, 10},
	{ {22, 4, 0}, 4},
	{ {21, 4, 0}, 4},
	{ {1, 2, 0}, 2},
	{ {4, 1, 0}, 9},
	{ {8, 4, 0}, 12},
	{ {12, 4, 0}, 16}
};

static TemplateInfo ip_traffic_template = {
	7,
	ip_traffic_fi,
	NULL
};

static TemplateInfo icmp_traffic_template = {
	8,
	icmp_traffic_fi,
	NULL
};

static TemplateInfo tcpudp_traffic_template = {
	9,
	tcpudp_traffic_fi,
	NULL
};

/*
 this is the function called by sampler::HookingFilter
 the entrypoint into the concentrator
 this is a mess and not thread safe
 */

void sampler_hook_entry(void *ctx, void *data)
{
	int transport_offset;
	struct packet_hook *ph=(struct packet_hook *)data;
        void *aggregator=ctx;
	FieldData *fdata=(FieldData *)ph->ip_header;
	uint32_t pad1;
	uint8_t pad2;
	
	//DPRINTF("hook_entry: length is %d\n", ph->length);

	/* save IP header */
	pad1=((uint32_t *)ph->ip_header)[1];
	pad2=((uint16_t *)ph->ip_header)[5];
	((uint32_t *)ph->ip_header)[1]=htonl((uint32_t)ph->timestamp->tv_sec);
	((uint8_t *)ph->ip_header)[10]=(uint8_t)1;

	switch(((char *)ph->ip_header)[9]) {
	case IPFIX_protocolIdentifier_ICMP:
		/*
	 	because of IP options we need to re-calculate the offsets to srcport and dstport every time
	 	now we do need some serious pointer arithmetic:
	 	- calculate the offset of transport header to ip header
	 	- use this offset and add to src/dst_port offset
	 	*/
		transport_offset=abs(ph->transport_header - ph->ip_header);
		icmp_traffic_template.fieldInfo[0].offset += transport_offset;
		aggregateDataRecord(aggregator, HOOK_SOURCE_ID, &icmp_traffic_template, ph->length, fdata);
		/* reset ip_traffic_fi, offsets for srcport/dstport to starting values */
		tcpudp_traffic_template.fieldInfo[0].offset = 0;
		break;
	case IPFIX_protocolIdentifier_TCP:
	case IPFIX_protocolIdentifier_UDP:
		/*
	 	because of IP options we need to re-calculate the offsets to srcport and dstport every time
	 	now we do need some serious pointer arithmetic:
	 	- calculate the offset of transport header to ip header
	 	- use this offset and add to src/dst_port offset
	 	*/
		transport_offset=abs(ph->transport_header - ph->ip_header);
		tcpudp_traffic_template.fieldInfo[0].offset += transport_offset;
		tcpudp_traffic_template.fieldInfo[1].offset += transport_offset;
		aggregateDataRecord(aggregator, HOOK_SOURCE_ID, &tcpudp_traffic_template, ph->length, fdata);
		/* reset ip_traffic_fi, offsets for srcport/dstport to starting values */
		tcpudp_traffic_template.fieldInfo[0].offset = 0;
		tcpudp_traffic_template.fieldInfo[1].offset = 2;
		break;
	default:
		aggregateDataRecord(aggregator, HOOK_SOURCE_ID, &ip_traffic_template, ph->length, fdata);
	}

	/* restore IP header */
	((uint32_t *)ph->ip_header)[1]=pad1;
	((uint8_t *)ph->ip_header)[10]=pad2;


}

#ifdef __cplusplus
}
#endif
