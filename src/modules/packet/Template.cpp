/*
 * PSAMP Reference Implementation
 * Copyright (C) 2014 Oliver Gasser
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


bool Template::addField(const InformationElement::IeInfo& ie)
{
	uint16_t offset;
	uint16_t header = 0;
	uint32_t validPacketClass = 0;

	/* time stamps are not in the captured packet bytes, but in separate member variables of the Packet class */
	if ((ie == IeInfo(IPFIX_TYPEID_flowStartSeconds, 0, IPFIX_LENGTH_flowStartSeconds)) // length of timeval.tv_sec
			|| (ie == IeInfo(IPFIX_TYPEID_flowStartMilliseconds, 0, IPFIX_LENGTH_flowStartMilliseconds))
			|| (ie == IeInfo(IPFIX_TYPEID_flowStartMicroseconds, 0, IPFIX_LENGTH_flowStartMicroseconds))
			|| (ie == IeInfo(PSAMP_TYPEID_observationTimeSeconds, 0, PSAMP_LENGTH_observationTimeSeconds))
			|| (ie == IeInfo(PSAMP_TYPEID_observationTimeMilliseconds, 0, PSAMP_LENGTH_observationTimeMilliseconds))
			|| (ie == IeInfo(PSAMP_TYPEID_observationTimeMicroseconds, 0, PSAMP_LENGTH_observationTimeMicroseconds)))
	{
		addFieldWithoutOffset(ie);
		if ((ie == IeInfo(IPFIX_TYPEID_flowStartMicroseconds, 0)) || (ie == IeInfo(PSAMP_TYPEID_observationTimeMicroseconds, 0)))
			msg(MSG_DIALOG, "Warning! %s encoded as complement for flowStartSeconds/observationTimeSeconds (deviating from IPFIX info model).", ie.toString().c_str());
	}
	else
	{
		if(getFieldOffsetAndHeader(ie, &offset, &header, &validPacketClass) == false)
		{
			msg(MSG_DIALOG, "Unsupported %s will be ignored by PSAMP exporter.", ie.toString().c_str());
			return false;
		}

		addFieldWithOffset(ie, offset, header, validPacketClass);
	}

	return true;
}



bool Template::getFieldOffsetAndHeader(const IeInfo& ie, uint16_t *offset, uint16_t *header, uint32_t *validPacketClass)
{
	if (ie.enterprise==0) {
		switch(ie.id) {
			// IPv4/6 *header fields:
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
				*validPacketClass = PCLASS_NET_IP4 | PCLASS_NET_IP6;
				break;
            case IPFIX_TYPEID_ipClassOfService:
				*offset=1;
				*header=HEAD_NETWORK;
				*validPacketClass = PCLASS_NET_IP4 | PCLASS_NET_IP6;
				break;
			case IPFIX_TYPEID_fragmentIdentification:
				*offset=4;
				*header=HEAD_NETWORK;
				*validPacketClass = PCLASS_NET_IP4 | PCLASS_NET_IP6;
				break;
			case IPFIX_TYPEID_fragmentOffset:
				// note: no masking, you also get DF and MF flags
				*offset=6;
				*header=HEAD_NETWORK;
				*validPacketClass = PCLASS_NET_IP4 | PCLASS_NET_IP6;
				break;
			case IPFIX_TYPEID_ipTTL:
				*offset=8;
				*header=HEAD_NETWORK;
				*validPacketClass = PCLASS_NET_IP4 | PCLASS_NET_IP6;
				break;
			case IPFIX_TYPEID_totalLengthIPv4:
				*offset=2;
				*header=HEAD_NETWORK;
				*validPacketClass = PCLASS_NET_IP4;
				break;

				// ICMP/IGMP header fields:
			case IPFIX_TYPEID_icmpTypeCodeIPv4:
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
			case IPFIX_TYPEID_flowLabelIPv6:
			default:
				return false;
		}
	} else if (ie.enterprise == IPFIX_PEN_vermont) {
		switch(ie.id) {
			// Transport layer payload
			case IPFIX_ETYPEID_frontPayload:
				*offset=0;
				*header=HEAD_PAYLOAD;
				*validPacketClass = PCLASS_PAYLOAD; // any payload
				break;

			default:
				return false;
		}
	} else {
		return false;
	}

	return true;
}
