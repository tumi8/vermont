/*
 this is vermont.
 released under GPL v2

 (C) by Ronny T. Lampert
 */

#define HOOK_SOURCE_ID 4913
/* just for abs */
#include <stdlib.h>
/* be sure we get the htonl et al inlined */
#include <netinet/in.h>

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
	/* { { ID, len, enterprise}, offset} */
	{ {IPFIX_TYPEID_packetDeltaCount,         1, 0}, 10},
	{ {IPFIX_TYPEID_flowStartSeconds,         4, 0}, 4},
	{ {IPFIX_TYPEID_flowEndSeconds,           4, 0}, 4},
	{ {IPFIX_TYPEID_octetDeltaCount,          2, 0}, 2},
	{ {IPFIX_TYPEID_protocolIdentifier,       1, 0}, 9},
	{ {IPFIX_TYPEID_sourceIPv4Address,        4, 0}, 12},
	{ {IPFIX_TYPEID_destinationIPv4Address,   4, 0}, 16}
};

static FieldInfo icmp_traffic_fi[] = {
	/* { { ID, len, enterprise}, offset} */
	{ {IPFIX_TYPEID_icmpTypeCode,             2, 0}, 0},
	{ {IPFIX_TYPEID_packetDeltaCount,         1, 0}, 10},
	{ {IPFIX_TYPEID_flowStartSeconds,         4, 0}, 4},
	{ {IPFIX_TYPEID_flowEndSeconds,           4, 0}, 4},
	{ {IPFIX_TYPEID_octetDeltaCount,          2, 0}, 2},
	{ {IPFIX_TYPEID_protocolIdentifier,       1, 0}, 9},
	{ {IPFIX_TYPEID_sourceIPv4Address,        4, 0}, 12},
	{ {IPFIX_TYPEID_destinationIPv4Address,   4, 0}, 16}
};

static FieldInfo udp_traffic_fi[] = {
	/* { { ID, len, enterprise}, offset} */
	{ {IPFIX_TYPEID_sourceTransportPort,      2, 0}, 0},
	{ {IPFIX_TYPEID_destinationTransportPort, 2, 0}, 2},
	{ {IPFIX_TYPEID_packetDeltaCount,         1, 0}, 10},
	{ {IPFIX_TYPEID_flowStartSeconds,         4, 0}, 4},
	{ {IPFIX_TYPEID_flowEndSeconds,           4, 0}, 4},
	{ {IPFIX_TYPEID_octetDeltaCount,          2, 0}, 2},
	{ {IPFIX_TYPEID_protocolIdentifier,       1, 0}, 9},
	{ {IPFIX_TYPEID_sourceIPv4Address,        4, 0}, 12},
	{ {IPFIX_TYPEID_destinationIPv4Address,   4, 0}, 16}
};

static FieldInfo tcp_traffic_fi[] = {
	/* { { ID, len, enterprise}, offset} */
	{ {IPFIX_TYPEID_tcpControlBits,           1, 0}, 13},
	{ {IPFIX_TYPEID_sourceTransportPort,      2, 0}, 0},
	{ {IPFIX_TYPEID_destinationTransportPort, 2, 0}, 2},
	{ {IPFIX_TYPEID_packetDeltaCount,         1, 0}, 10},
	{ {IPFIX_TYPEID_flowStartSeconds,         4, 0}, 4},
	{ {IPFIX_TYPEID_flowEndSeconds,           4, 0}, 4},
	{ {IPFIX_TYPEID_octetDeltaCount,          2, 0}, 2},
	{ {IPFIX_TYPEID_protocolIdentifier,       1, 0}, 9},
	{ {IPFIX_TYPEID_sourceIPv4Address,        4, 0}, 12},
	{ {IPFIX_TYPEID_destinationIPv4Address,   4, 0}, 16}
};

static TemplateInfo ip_traffic_template = {
	.fieldCount = 7,
	.fieldInfo  = ip_traffic_fi,
	.userData   = NULL
};

static TemplateInfo icmp_traffic_template = {
	.fieldCount = 8,
	.fieldInfo  = icmp_traffic_fi,
	.userData   = NULL
};

static TemplateInfo udp_traffic_template = {
	.fieldCount = 9,
	.fieldInfo  = udp_traffic_fi,
	.userData   = NULL
};

static TemplateInfo tcp_traffic_template = {
	.fieldCount = 10,
	.fieldInfo  = tcp_traffic_fi,
	.userData   = NULL
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

	// Check if transport header is available
	if(ph->transport_header == NULL) {
	    aggregateDataRecord(aggregator, HOOK_SOURCE_ID, &ip_traffic_template, ph->length, fdata);
	}
	else
	{
	    // Choose template according to transport header type
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
		    /* reset offset for typecode to starting value */
		    icmp_traffic_template.fieldInfo[0].offset = 0;
		    break;
		case IPFIX_protocolIdentifier_UDP:
		    /*
		       because of IP options we need to re-calculate the offsets to srcport and dstport every time
		       now we do need some serious pointer arithmetic:
		       - calculate the offset of transport header to ip header
		       - use this offset and add to src/dst_port offset
		     */
		    transport_offset=abs(ph->transport_header - ph->ip_header);
		    udp_traffic_template.fieldInfo[0].offset += transport_offset;
		    udp_traffic_template.fieldInfo[1].offset += transport_offset;
		    aggregateDataRecord(aggregator, HOOK_SOURCE_ID, &udp_traffic_template, ph->length, fdata);
		    /* reset offsets for srcport/dstport to starting values */
		    udp_traffic_template.fieldInfo[0].offset = 0;
		    udp_traffic_template.fieldInfo[1].offset = 2;
		    break;
		case IPFIX_protocolIdentifier_TCP:
		    /*
		       because of IP options we need to re-calculate the offsets to srcport and dstport every time
		       now we do need some serious pointer arithmetic:
		       - calculate the offset of transport header to ip header
		       - use this offset and add to src/dst_port offset
		     */
		    transport_offset=abs(ph->transport_header - ph->ip_header);
		    tcp_traffic_template.fieldInfo[0].offset += transport_offset;
		    tcp_traffic_template.fieldInfo[1].offset += transport_offset;
		    aggregateDataRecord(aggregator, HOOK_SOURCE_ID, &tcp_traffic_template, ph->length, fdata);
		    /* reset offsets for srcport/dstport to starting values */
		    tcp_traffic_template.fieldInfo[0].offset = 0;
		    tcp_traffic_template.fieldInfo[1].offset = 2;
		    break;
		default:
		    aggregateDataRecord(aggregator, HOOK_SOURCE_ID, &ip_traffic_template, ph->length, fdata);
	    }
	}

	/* restore IP header */
	((uint32_t *)ph->ip_header)[1]=pad1;
	((uint8_t *)ph->ip_header)[10]=pad2;

}

#ifdef __cplusplus
}
#endif
