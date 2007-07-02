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

#include "common/msg.h"

#include <iostream>


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
	ip_traffic_fi = new IpfixRecord::FieldInfo[9]();
	//ip_traffic_fi = (IpfixRecord::FieldInfo*)malloc(7 * sizeof(IpfixRecord::FieldInfo));
	setFieldInfo(&ip_traffic_fi[0], IPFIX_TYPEID_packetDeltaCount, 1, 10);
	setFieldInfo(&ip_traffic_fi[1], IPFIX_TYPEID_flowStartSeconds, 4, 4); // see processPacket() for explanation of how times are treated
	setFieldInfo(&ip_traffic_fi[2], IPFIX_TYPEID_flowEndSeconds, 4, 4);
	setFieldInfo(&ip_traffic_fi[3], IPFIX_TYPEID_octetDeltaCount, 2, 2);
	setFieldInfo(&ip_traffic_fi[4], IPFIX_TYPEID_protocolIdentifier, 1, 9);
	setFieldInfo(&ip_traffic_fi[5], IPFIX_TYPEID_sourceIPv4Address, 4, 12);
	setFieldInfo(&ip_traffic_fi[6], IPFIX_TYPEID_destinationIPv4Address, 4, 16);
	setFieldInfo(&ip_traffic_fi[7], IPFIX_TYPEID_flowStartMilliSeconds, 8, 0);
	setFieldInfo(&ip_traffic_fi[8], IPFIX_TYPEID_flowEndMilliSeconds, 8, 0);

	icmp_traffic_fi = new IpfixRecord::FieldInfo[10];
	setFieldInfo(&icmp_traffic_fi[0], IPFIX_TYPEID_icmpTypeCode, 2, 0);
	setFieldInfo(&icmp_traffic_fi[1], IPFIX_TYPEID_packetDeltaCount, 1, 10);
	setFieldInfo(&icmp_traffic_fi[2], IPFIX_TYPEID_flowStartSeconds, 4, 4);
	setFieldInfo(&icmp_traffic_fi[3], IPFIX_TYPEID_flowEndSeconds, 4, 4);
	setFieldInfo(&icmp_traffic_fi[4], IPFIX_TYPEID_octetDeltaCount, 2, 2);
	setFieldInfo(&icmp_traffic_fi[5], IPFIX_TYPEID_protocolIdentifier, 1, 9);
	setFieldInfo(&icmp_traffic_fi[6], IPFIX_TYPEID_sourceIPv4Address, 4, 12);
	setFieldInfo(&icmp_traffic_fi[7], IPFIX_TYPEID_destinationIPv4Address, 4, 16);
	setFieldInfo(&icmp_traffic_fi[8], IPFIX_TYPEID_flowStartMilliSeconds, 8, 0);
	setFieldInfo(&icmp_traffic_fi[9], IPFIX_TYPEID_flowEndMilliSeconds, 8, 0);

	udp_traffic_fi = new IpfixRecord::FieldInfo[11];
	setFieldInfo(&udp_traffic_fi[0], IPFIX_TYPEID_sourceTransportPort, 2, 0);
	setFieldInfo(&udp_traffic_fi[1], IPFIX_TYPEID_destinationTransportPort, 2, 2);
	setFieldInfo(&udp_traffic_fi[2], IPFIX_TYPEID_packetDeltaCount, 1, 10);
	setFieldInfo(&udp_traffic_fi[3], IPFIX_TYPEID_flowStartSeconds, 4, 4);
	setFieldInfo(&udp_traffic_fi[4], IPFIX_TYPEID_flowEndSeconds, 4, 4);
	setFieldInfo(&udp_traffic_fi[5], IPFIX_TYPEID_octetDeltaCount, 2, 2);
	setFieldInfo(&udp_traffic_fi[6], IPFIX_TYPEID_protocolIdentifier, 1, 9);
	setFieldInfo(&udp_traffic_fi[7], IPFIX_TYPEID_sourceIPv4Address, 4, 12);
	setFieldInfo(&udp_traffic_fi[8], IPFIX_TYPEID_destinationIPv4Address, 4, 16);
	setFieldInfo(&udp_traffic_fi[9], IPFIX_TYPEID_flowStartMilliSeconds, 8, 0);
	setFieldInfo(&udp_traffic_fi[10], IPFIX_TYPEID_flowEndMilliSeconds, 8, 0);

	tcp_traffic_fi = new IpfixRecord::FieldInfo[12];
	setFieldInfo(&tcp_traffic_fi[0], IPFIX_TYPEID_tcpControlBits, 1, 13);  		// is relative to start of TCP header
	setFieldInfo(&tcp_traffic_fi[1], IPFIX_TYPEID_sourceTransportPort, 2, 0);
	setFieldInfo(&tcp_traffic_fi[2], IPFIX_TYPEID_destinationTransportPort, 2, 2);
	setFieldInfo(&tcp_traffic_fi[3], IPFIX_TYPEID_packetDeltaCount, 1, 10);
	setFieldInfo(&tcp_traffic_fi[4], IPFIX_TYPEID_flowStartSeconds, 4, 4);
	setFieldInfo(&tcp_traffic_fi[5], IPFIX_TYPEID_flowEndSeconds, 4, 4);
	setFieldInfo(&tcp_traffic_fi[6], IPFIX_TYPEID_octetDeltaCount, 2, 2);
	setFieldInfo(&tcp_traffic_fi[7], IPFIX_TYPEID_protocolIdentifier, 1, 9);
	setFieldInfo(&tcp_traffic_fi[8], IPFIX_TYPEID_sourceIPv4Address, 4, 12);
	setFieldInfo(&tcp_traffic_fi[9], IPFIX_TYPEID_destinationIPv4Address, 4, 16);
	setFieldInfo(&tcp_traffic_fi[10], IPFIX_TYPEID_flowStartMilliSeconds, 8, 0);
	setFieldInfo(&tcp_traffic_fi[11], IPFIX_TYPEID_flowEndMilliSeconds, 8, 0);

	ip_traffic_template = new IpfixRecord::TemplateInfo();
	ip_traffic_template->templateId = 0;
	ip_traffic_template->fieldCount = 9;
	ip_traffic_template->fieldInfo  = ip_traffic_fi;
	ip_traffic_template->userData   = 0;

	icmp_traffic_template = new IpfixRecord::TemplateInfo();
	icmp_traffic_template->templateId = 0;
	icmp_traffic_template->fieldCount = 10;
	icmp_traffic_template->fieldInfo  = icmp_traffic_fi;
	icmp_traffic_template->userData   = 0;

	udp_traffic_template = new IpfixRecord::TemplateInfo();
	udp_traffic_template->templateId = 0;
	udp_traffic_template->fieldCount = 11;
	udp_traffic_template->fieldInfo  = udp_traffic_fi;
	udp_traffic_template->userData   = 0;

	tcp_traffic_template = new IpfixRecord::TemplateInfo();
	tcp_traffic_template->templateId = 0;
	tcp_traffic_template->fieldCount = 12;
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
   	DPRINTFL(MSG_VDEBUG, "called");
	/* we want only IPv4 packets  */
	if((p->classification & PCLASS_NET_IP4) == 0)
	    return true;
	    
	IpfixRecord::Data *fdata=(IpfixRecord::Data *)p->netHeader;
	uint32_t pad1;
	uint8_t pad2;



	//DPRINTF("hook_entry: length is %d\n", p->data_length);

	/* save IP header */
	pad1=((uint32_t *)p->netHeader)[1];
	pad2=((uint8_t *)p->netHeader)[10];

	// save the current time inside the IP header and overwrite
	// identification, flags and fragment offset fields
	((uint32_t *)p->netHeader)[1]=p->time_sec_nbo;
	((uint8_t *)p->netHeader)[10]=(uint8_t)1;

	// now perform some really BAD hack for the millisecond time:
	// our problem is that at the moment only the raw packet data is transferred to the concentrator
	// and there is no place for our 16 bytes of time data; additionally, in the template only indizes
	// relative to the start of the network packet data is given
	// now we calculate the index for the millisecond data inside the Packet instance
	// -> REALLY BAD HACK, interface to concentrator MUST be changed
	char* mstime = (char*)(&p->time_msec_ipfix);
        DPRINTF("time_msec_ipfix is %llX", p->time_msec_ipfix);
	int32_t offset = (int32_t)(mstime - (char*)(p->netHeader)); 
        DPRINTF("offset=%x", offset);

	// Choose template according to transport header type
	if(p->ipProtocolType == Packet::ICMP) {
	    /* adapt offset for millisecond timestamps */
	    icmp_traffic_template->fieldInfo[8].offset = offset;
	    icmp_traffic_template->fieldInfo[9].offset = offset;
	    /* adapt offset for typecode */
	    icmp_traffic_template->fieldInfo[0].offset += (p->transportHeaderOffset - p->netHeaderOffset);
	    flowSink->onDataRecord(NULL, icmp_traffic_template, p->data_length, fdata);
	    /* reset offset for typecode to starting value */
	    icmp_traffic_template->fieldInfo[0].offset = 0;

	} else if(p->ipProtocolType == Packet::UDP) {
	    /* adapt offset for millisecond timestamps */
	    udp_traffic_template->fieldInfo[9].offset = offset;
	    udp_traffic_template->fieldInfo[10].offset = offset;
	    /* adapt offsets for srcport/dstport */
	    udp_traffic_template->fieldInfo[0].offset += (p->transportHeaderOffset - p->netHeaderOffset);
	    udp_traffic_template->fieldInfo[1].offset += (p->transportHeaderOffset - p->netHeaderOffset);
	    flowSink->onDataRecord(NULL, udp_traffic_template, p->data_length, fdata);
	    /* reset offsets for srcport/dstport to starting values */
	    udp_traffic_template->fieldInfo[0].offset = 0;
	    udp_traffic_template->fieldInfo[1].offset = 2;

	} else if(p->ipProtocolType == Packet::TCP) {
	    /* adapt offset for millisecond timestamps */
	    tcp_traffic_template->fieldInfo[10].offset = offset;
	    tcp_traffic_template->fieldInfo[11].offset = offset;
	    /* adapt offsets for srcport/dstport, tcpcontrolbits */
	    tcp_traffic_template->fieldInfo[0].offset += (p->transportHeaderOffset - p->netHeaderOffset);
	    tcp_traffic_template->fieldInfo[1].offset += (p->transportHeaderOffset - p->netHeaderOffset);
	    tcp_traffic_template->fieldInfo[2].offset += (p->transportHeaderOffset - p->netHeaderOffset);
	    flowSink->onDataRecord(NULL, tcp_traffic_template, p->data_length, fdata);
	    /* reset offsets for srcport/dstport, tcpcontrolbits to starting values */
	    tcp_traffic_template->fieldInfo[0].offset = 13;
	    tcp_traffic_template->fieldInfo[1].offset = 0;
	    tcp_traffic_template->fieldInfo[2].offset = 2;

	} else {
	    /* adapt offset for millisecond timestamps */
	    ip_traffic_template->fieldInfo[7].offset = offset;
	    ip_traffic_template->fieldInfo[8].offset = offset;
	    /* IP only */
	    flowSink->onDataRecord(NULL, ip_traffic_template, p->data_length, fdata);
	}
	
	/* restore IP header */
	((uint32_t *)p->netHeader)[1]=pad1;
	((uint8_t *)p->netHeader)[10]=pad2;

	return true;
}


