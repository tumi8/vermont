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


namespace {
	void setFieldInfo(IpfixRecord::FieldInfo* fi, IpfixRecord::FieldInfo::Type::Id id, IpfixRecord::FieldInfo::Type::Length length, uint16_t offset) {
		fi->type.id = id;
		fi->type.length = length;
		fi->type.isVariableLength = 0;
		fi->type.eid = 0;
		fi->offset = offset; 
	}
}

HookingFilter::HookingFilter(FlowSink *flowSink) : flowSink(flowSink) {
	ip_traffic_fi = (IpfixRecord::FieldInfo*)malloc(7 * sizeof(IpfixRecord::FieldInfo));
	setFieldInfo(&ip_traffic_fi[0], IPFIX_TYPEID_packetDeltaCount, 1, 10);
	setFieldInfo(&ip_traffic_fi[1], IPFIX_TYPEID_flowStartSeconds, 4, 4);
	setFieldInfo(&ip_traffic_fi[2], IPFIX_TYPEID_flowEndSeconds, 4, 4);
	setFieldInfo(&ip_traffic_fi[3], IPFIX_TYPEID_octetDeltaCount, 2, 2);
	setFieldInfo(&ip_traffic_fi[4], IPFIX_TYPEID_protocolIdentifier, 1, 9);
	setFieldInfo(&ip_traffic_fi[5], IPFIX_TYPEID_sourceIPv4Address, 4, 12);
	setFieldInfo(&ip_traffic_fi[6], IPFIX_TYPEID_destinationIPv4Address, 4, 16);

	icmp_traffic_fi = (IpfixRecord::FieldInfo*)malloc(8 * sizeof(IpfixRecord::FieldInfo));
	setFieldInfo(&icmp_traffic_fi[0], IPFIX_TYPEID_icmpTypeCode, 2, 0);
	setFieldInfo(&icmp_traffic_fi[1], IPFIX_TYPEID_packetDeltaCount, 1, 10);
	setFieldInfo(&icmp_traffic_fi[2], IPFIX_TYPEID_flowStartSeconds, 4, 4);
	setFieldInfo(&icmp_traffic_fi[3], IPFIX_TYPEID_flowEndSeconds, 4, 4);
	setFieldInfo(&icmp_traffic_fi[4], IPFIX_TYPEID_octetDeltaCount, 2, 2);
	setFieldInfo(&icmp_traffic_fi[5], IPFIX_TYPEID_protocolIdentifier, 1, 9);
	setFieldInfo(&icmp_traffic_fi[6], IPFIX_TYPEID_sourceIPv4Address, 4, 12);
	setFieldInfo(&icmp_traffic_fi[7], IPFIX_TYPEID_destinationIPv4Address, 4, 16);

	udp_traffic_fi = (IpfixRecord::FieldInfo*)malloc(9 * sizeof(IpfixRecord::FieldInfo));
	setFieldInfo(&udp_traffic_fi[0], IPFIX_TYPEID_sourceTransportPort, 2, 0);
	setFieldInfo(&udp_traffic_fi[1], IPFIX_TYPEID_destinationTransportPort, 2, 2);
	setFieldInfo(&udp_traffic_fi[2], IPFIX_TYPEID_packetDeltaCount, 1, 10);
	setFieldInfo(&udp_traffic_fi[3], IPFIX_TYPEID_flowStartSeconds, 4, 4);
	setFieldInfo(&udp_traffic_fi[4], IPFIX_TYPEID_flowEndSeconds, 4, 4);
	setFieldInfo(&udp_traffic_fi[5], IPFIX_TYPEID_octetDeltaCount, 2, 2);
	setFieldInfo(&udp_traffic_fi[6], IPFIX_TYPEID_protocolIdentifier, 1, 9);
	setFieldInfo(&udp_traffic_fi[7], IPFIX_TYPEID_sourceIPv4Address, 4, 12);
	setFieldInfo(&udp_traffic_fi[8], IPFIX_TYPEID_destinationIPv4Address, 4, 16);

	tcp_traffic_fi = (IpfixRecord::FieldInfo*)malloc(10 * sizeof(IpfixRecord::FieldInfo));
	setFieldInfo(&tcp_traffic_fi[0], IPFIX_TYPEID_tcpControlBits, 1, 13);
	setFieldInfo(&tcp_traffic_fi[1], IPFIX_TYPEID_sourceTransportPort, 2, 0);
	setFieldInfo(&tcp_traffic_fi[2], IPFIX_TYPEID_destinationTransportPort, 2, 2);
	setFieldInfo(&tcp_traffic_fi[3], IPFIX_TYPEID_packetDeltaCount, 1, 10);
	setFieldInfo(&tcp_traffic_fi[4], IPFIX_TYPEID_flowStartSeconds, 4, 4);
	setFieldInfo(&tcp_traffic_fi[5], IPFIX_TYPEID_flowEndSeconds, 4, 4);
	setFieldInfo(&tcp_traffic_fi[6], IPFIX_TYPEID_octetDeltaCount, 2, 2);
	setFieldInfo(&tcp_traffic_fi[7], IPFIX_TYPEID_protocolIdentifier, 1, 9);
	setFieldInfo(&tcp_traffic_fi[8], IPFIX_TYPEID_sourceIPv4Address, 4, 12);
	setFieldInfo(&tcp_traffic_fi[9], IPFIX_TYPEID_destinationIPv4Address, 4, 16);

	ip_traffic_template = new IpfixRecord::TemplateInfo();
	ip_traffic_template->templateId = 0;
	ip_traffic_template->fieldCount = 7;
	ip_traffic_template->fieldInfo  = ip_traffic_fi;
	ip_traffic_template->userData   = 0;

	icmp_traffic_template = new IpfixRecord::TemplateInfo();
	icmp_traffic_template->templateId = 0;
	icmp_traffic_template->fieldCount = 8;
	icmp_traffic_template->fieldInfo  = icmp_traffic_fi;
	icmp_traffic_template->userData   = 0;

	udp_traffic_template = new IpfixRecord::TemplateInfo();
	udp_traffic_template->templateId = 0;
	udp_traffic_template->fieldCount = 9;
	udp_traffic_template->fieldInfo  = udp_traffic_fi;
	udp_traffic_template->userData   = 0;

	tcp_traffic_template = new IpfixRecord::TemplateInfo();
	tcp_traffic_template->templateId = 0;
	tcp_traffic_template->fieldCount = 10;
	tcp_traffic_template->fieldInfo  = tcp_traffic_fi;
	tcp_traffic_template->userData   = 0; 
}

HookingFilter::~HookingFilter() {
	delete ip_traffic_template;
	delete icmp_traffic_template;
	delete udp_traffic_template;
	delete tcp_traffic_template;
}

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
	    flowSink->onDataRecord(NULL, ip_traffic_template, p->data_length, fdata);
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
		    icmp_traffic_template->fieldInfo[0].offset += transport_offset;
		    flowSink->onDataRecord(NULL, icmp_traffic_template, p->data_length, fdata);
		    /* reset offset for typecode to starting value */
		    icmp_traffic_template->fieldInfo[0].offset = 0;
		    break;
		case IPFIX_protocolIdentifier_UDP:
		    /*
		       because of IP options we need to re-calculate the offsets to srcport and dstport every time
		       now we do need some serious pointer arithmetic:
		       - calculate the offset of transport header to ip header
		       - use this offset and add to src/dst_port offset
		     */
		    transport_offset=abs((char*)p->transportHeader - (char*)p->netHeader);
		    udp_traffic_template->fieldInfo[0].offset += transport_offset;
		    udp_traffic_template->fieldInfo[1].offset += transport_offset;
		    flowSink->onDataRecord(NULL, udp_traffic_template, p->data_length, fdata);
		    /* reset offsets for srcport/dstport to starting values */
		    udp_traffic_template->fieldInfo[0].offset = 0;
		    udp_traffic_template->fieldInfo[1].offset = 2;
		    break;
		case IPFIX_protocolIdentifier_TCP:
		    /*
		       because of IP options we need to re-calculate the offsets to srcport and dstport every time
		       now we do need some serious pointer arithmetic:
		       - calculate the offset of transport header to ip header
		       - use this offset and add to src/dst_port offset
		     */
		    transport_offset=abs((char*)p->transportHeader - (char*)p->netHeader);
		    tcp_traffic_template->fieldInfo[0].offset += transport_offset;
		    tcp_traffic_template->fieldInfo[1].offset += transport_offset;
		    tcp_traffic_template->fieldInfo[2].offset += transport_offset;
		    flowSink->onDataRecord(NULL, tcp_traffic_template, p->data_length, fdata);
		    /* reset offsets for srcport/dstport to starting values */
		    tcp_traffic_template->fieldInfo[0].offset = 13;
		    tcp_traffic_template->fieldInfo[1].offset = 0;
		    tcp_traffic_template->fieldInfo[2].offset = 2;
		    break;
		default:
		    flowSink->onDataRecord(NULL, ip_traffic_template, p->data_length, fdata);
	    }
	}

	/* restore IP header */
	((uint32_t *)p->netHeader)[1]=pad1;
	((uint8_t *)p->netHeader)[10]=pad2;

	return true;
}


