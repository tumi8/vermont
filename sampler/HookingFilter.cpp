#define HOOK_SOURCE_ID 4913
/* just for abs */
#include <stdlib.h>
/* be sure we get the htonl et al inlined */
#include <netinet/in.h>

#include "HookingFilter.h"
#include "Packet.h"
#include "concentrator/ipfix.hpp"
#include "concentrator/IpfixAggregator.hpp"
#include "concentrator/IpfixParser.hpp"
#include "concentrator/IpfixPrinter.hpp"

#include "msg.h"

/*
 this is the function called by sampler::HookingFilter
 the entrypoint into the concentrator
 this is a mess and not thread safe
 */

bool HookingFilter::processPacket(const Packet *p)
{
	/* we want only IPv4 packets  */
	if((p->classification & PCLASS_NET_IP4) == 0)
	    return true;
	    
	int transport_offset;
	IpfixRecord::Data *fdata=(IpfixRecord::Data *)p->netHeader;
	uint32_t pad1;
	uint8_t pad2;
	
	//DPRINTF("hook_entry: length is %d\n", p->data_length);

	/* save IP header */
	pad1=((uint32_t *)p->netHeader)[1];
	pad2=((uint16_t *)p->netHeader)[5];
	((uint32_t *)p->netHeader)[1]=htonl((uint32_t)p->timestamp.tv_sec);
	((uint8_t *)p->netHeader)[10]=(uint8_t)1;

	// Check if transport header is available
	if(p->transportHeader == NULL) {
	    flowSink->onDataRecord(NULL, &ip_traffic_template, p->data_length, fdata);
	}
	else
	{
	    // Choose template according to transport header type
	    switch(((char *)p->netHeader)[9]) {
		case IPFIX_protocolIdentifier_ICMP:
		    /*
		       because of IP options we need to re-calculate the offsets to srcport and dstport every time
		       now we do need some serious pointer arithmetic:
		       - calculate the offset of transport header to ip header
		       - use this offset and add to src/dst_port offset
		     */
		    transport_offset=abs((char*)p->transportHeader - (char*)p->netHeader);
		    icmp_traffic_template.fieldInfo[0].offset += transport_offset;
		    flowSink->onDataRecord(NULL, &icmp_traffic_template, p->data_length, fdata);
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
		    transport_offset=abs((char*)p->transportHeader - (char*)p->netHeader);
		    udp_traffic_template.fieldInfo[0].offset += transport_offset;
		    udp_traffic_template.fieldInfo[1].offset += transport_offset;
		    flowSink->onDataRecord(NULL, &udp_traffic_template, p->data_length, fdata);
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
		    transport_offset=abs((char*)p->transportHeader - (char*)p->netHeader);
		    tcp_traffic_template.fieldInfo[0].offset += transport_offset;
		    tcp_traffic_template.fieldInfo[1].offset += transport_offset;
		    tcp_traffic_template.fieldInfo[2].offset += transport_offset;
		    flowSink->onDataRecord(NULL, &tcp_traffic_template, p->data_length, fdata);
		    /* reset offsets for srcport/dstport to starting values */
		    tcp_traffic_template.fieldInfo[0].offset = 13;
		    tcp_traffic_template.fieldInfo[1].offset = 0;
		    tcp_traffic_template.fieldInfo[2].offset = 2;
		    break;
		default:
		    flowSink->onDataRecord(NULL, &ip_traffic_template, p->data_length, fdata);
	    }
	}

	/* restore IP header */
	((uint32_t *)p->netHeader)[1]=pad1;
	((uint8_t *)p->netHeader)[10]=pad2;

	return true;
}

IpfixRecord::FieldInfo HookingFilter::ip_traffic_fi[] = {
	/* { { ID, len, enterprise}, offset} */
	{ {IPFIX_TYPEID_packetDeltaCount,         1, 0}, 10},
	{ {IPFIX_TYPEID_flowStartSeconds,         4, 0}, 4},
	{ {IPFIX_TYPEID_flowEndSeconds,           4, 0}, 4},
	{ {IPFIX_TYPEID_octetDeltaCount,          2, 0}, 2},
	{ {IPFIX_TYPEID_protocolIdentifier,       1, 0}, 9},
	{ {IPFIX_TYPEID_sourceIPv4Address,        4, 0}, 12},
	{ {IPFIX_TYPEID_destinationIPv4Address,   4, 0}, 16}
};

IpfixRecord::FieldInfo HookingFilter::icmp_traffic_fi[] = {
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

IpfixRecord::FieldInfo HookingFilter::udp_traffic_fi[] = {
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

IpfixRecord::FieldInfo HookingFilter::tcp_traffic_fi[] = {
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

IpfixRecord::TemplateInfo HookingFilter::ip_traffic_template = {
	/*.templateId =*/ 0,
	/*.fieldCount =*/ 7,
	/*.fieldInfo  =*/ ip_traffic_fi,
	/*.userData   =*/ NULL
};

IpfixRecord::TemplateInfo HookingFilter::icmp_traffic_template = {
	/*.templateId =*/ 0,
	/*.fieldCount =*/ 8,
	/*.fieldInfo  =*/ icmp_traffic_fi,
	/*.userData   =*/ NULL
};

IpfixRecord::TemplateInfo HookingFilter::udp_traffic_template = {
	/*.templateId =*/ 0,
	/*.fieldCount =*/ 9,
	/*.fieldInfo  =*/ udp_traffic_fi,
	/*.userData   =*/ NULL
};

IpfixRecord::TemplateInfo HookingFilter::tcp_traffic_template = {
	/*.templateId =*/ 0,
	/*.fieldCount =*/ 10,
	/*.fieldInfo  =*/ tcp_traffic_fi,
	/*.userData   =*/ NULL
};


