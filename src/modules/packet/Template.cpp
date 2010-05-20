/*
 * PSAMP Reference Implementation
 *
 * Template.cpp
 *
 * A Template definition
 *
 * Author: Michael Drueing <michael@drueing.de>
 *         Gerhard Muenz <gerhard.muenz@gmx.de>
 *
 */

#include <cstring>
#include <fstream>
#include <iostream>

#include "common/msg.h"

#include "common/ipfixlolib//ipfix.h"

#include "Template.h"


using namespace std;
using namespace InformationElement;


bool Template::addField(const InformationElement::IeInfo& id, uint16_t len)
{
	uint16_t offset;
	uint16_t header = 0;
	uint32_t validPacketClass = 0;

	/* it is a field with data from the packet itself */
	// TODO: This should really be checked against some kind of static array instead of a
	//       'switch' statement
	// Tobi during "PEN-fix": I really do not know what is done here. I hope i fixed it correctly
	if ((id.enterprise!=0)
			|| (id == IeInfo(IPFIX_TYPEID_flowStartSeconds, 0) && (len == sizeof(uint32_t))) // length of timeval.tv_sec
			|| (id == IeInfo(IPFIX_TYPEID_flowStartMicroSeconds, 0) && (len ==  sizeof(uint32_t)))
			|| (id == IeInfo(IPFIX_TYPEID_flowStartMilliSeconds, 0) && (len ==  sizeof(uint32_t long))))
	{
		addFieldWithoutOffset(id, len);
		if (id == IeInfo(IPFIX_TYPEID_flowStartMicroSeconds, 0))
			msg(MSG_DIALOG, "Warning! flowStartMicroSeconds used as complement for flowStartSeconds (deviating from IPFIX info model).");
	}
	else
	{
		if(getFieldOffsetAndHeader(id, &offset, &header, &validPacketClass) == false)
		{
			msg(MSG_ERROR, "ID %s currently not supported", id.toString().c_str());
			return false;
		}

		addFieldWithOffset(id, len, offset, header, validPacketClass);
	}

	return true;
}



bool Template::getFieldOffsetAndHeader(const IeInfo& id, uint16_t *offset, uint16_t *header, uint32_t *validPacketClass)
{
	if (id.enterprise==0) {
		switch(id.id) {
			// IPv4 *header fields:
			case IPFIX_TYPEID_sourceIPv4Address:
				*offset=12;
				*header=HEAD_NETWORK;
				*validPacketClass = PCLASS_NET_IP4;
				break;
			case IPFIX_TYPEID_destinationIPv4Address:
				*offset=16;
				*header=HEAD_NETWORK;
				*validPacketClass = PCLASS_NET_IP4;
				break;
			case IPFIX_TYPEID_protocolIdentifier:
				*offset=9;
				*header=HEAD_NETWORK;
				*validPacketClass = PCLASS_NET_IP4;
				break;
			case IPFIX_TYPEID_classOfServiceIPv4:
				*offset=1;
				*header=HEAD_NETWORK;
				*validPacketClass = PCLASS_NET_IP4;
				break;
			case IPFIX_TYPEID_identificationV4:
				*offset=4;
				*header=HEAD_NETWORK;
				*validPacketClass = PCLASS_NET_IP4;
				break;
			case IPFIX_TYPEID_fragmentOffsetIPv4:
				// note: no masking, you also get DF and MF flags
				*offset=6;
				*header=HEAD_NETWORK;
				*validPacketClass = PCLASS_NET_IP4;
				break;
			case IPFIX_TYPEID_ipTimeToLive:
				*offset=8;
				*header=HEAD_NETWORK;
				*validPacketClass = PCLASS_NET_IP4;
				break;
			case IPFIX_TYPEID_totalLengthIPv4:
				*offset=2;
				*header=HEAD_NETWORK;
				*validPacketClass = PCLASS_NET_IP4;
				break;

				// ICMP/IGMP header fields:
			case IPFIX_TYPEID_icmpTypeCode:
				*offset=0;
				*header=HEAD_TRANSPORT;
				*validPacketClass = PCLASS_TRN_ICMP;
				break;
			case IPFIX_TYPEID_igmpType:
				*offset=0;
				*header=HEAD_TRANSPORT;
				*validPacketClass = PCLASS_TRN_IGMP;
				break;

				// TCP/UDP header fields:
			case IPFIX_TYPEID_sourceTransportPort:
				*offset=0;
				*header=HEAD_TRANSPORT;
				*validPacketClass = PCLASS_TRN_TCP | PCLASS_TRN_UDP;
				break;
			case IPFIX_TYPEID_destinationTransportPort:
				*offset=2;
				*header=HEAD_TRANSPORT;
				*validPacketClass = PCLASS_TRN_TCP | PCLASS_TRN_UDP;
				break;
			case IPFIX_TYPEID_udpSourcePort:
				*offset=0;
				*header=HEAD_TRANSPORT;
				*validPacketClass = PCLASS_TRN_UDP;
				break;
			case IPFIX_TYPEID_udpDestinationPort:
				*offset=2;
				*header=HEAD_TRANSPORT;
				*validPacketClass = PCLASS_TRN_UDP;
				break;
			case IPFIX_TYPEID_tcpSourcePort:
				*offset=0;
				*header=HEAD_TRANSPORT;
				*validPacketClass = PCLASS_TRN_TCP;
				break;
			case IPFIX_TYPEID_tcpDestinationPort:
				*offset=2;
				*header=HEAD_TRANSPORT;
				*validPacketClass = PCLASS_TRN_TCP;
				break;
			case IPFIX_TYPEID_tcpControlBits:
				*offset=13;
				*header=HEAD_TRANSPORT;
				*validPacketClass = PCLASS_TRN_TCP;
				break;
			case IPFIX_TYPEID_tcpSequenceNumber:
				*offset=4;
				*header=HEAD_TRANSPORT;
				*validPacketClass = PCLASS_TRN_TCP;
				break;
			case IPFIX_TYPEID_tcpAcknowledgementNumber:
				*offset=8;
				*header=HEAD_TRANSPORT;
				*validPacketClass = PCLASS_TRN_TCP;
				break;
			case IPFIX_TYPEID_tcpWindowSize:
				*offset=14;
				*header=HEAD_TRANSPORT;
				*validPacketClass = PCLASS_TRN_TCP;
				break;
			case IPFIX_TYPEID_tcpUrgentPointer:
				*offset=18;
				*header=HEAD_TRANSPORT;
				*validPacketClass = PCLASS_TRN_TCP;
				break;

				// Header/payload dump fields:
			case PSAMP_TYPEID_ipHeaderPacketSection:
				*offset=0;
				*header=HEAD_NETWORK_AND_BEYOND;
				*validPacketClass = PCLASS_NET_IP4 | PCLASS_NET_IP6;
				break;
			case PSAMP_TYPEID_ipPayloadPacketSection:
				*offset=0;
				*header=HEAD_TRANSPORT_AND_BEYOND;
				*validPacketClass = PCLASS_TRNMASK | PCLASS_PAYLOAD; // any transport header or payload
				break;


			case IPFIX_TYPEID_ipVersion:
				// TODO: check how to export 4 bits

				// currently, only IPv4 is supported, fill in the following lines if needed
			case IPFIX_TYPEID_sourceIPv6Address:
			case IPFIX_TYPEID_destinationIPv6Address:
			case IPFIX_TYPEID_classOfServiceV6:
			case IPFIX_TYPEID_flowLabelV6:
			default:
				return false;
		}
	} else {
		return false;
	}

	return true;
}
