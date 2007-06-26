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

#include "concentrator/ipfix.hpp"

#include "Template.h"


using namespace std;


bool Template::addField(uint16_t id, uint16_t len)
{
        uint16_t offset;
	unsigned short header = 0;
	unsigned long validPacketClass = 0;

	/* it is a field with data from the packet itself */
	// TODO: This should really be checked against some kind of static array instead of a
	//       'switch' statement
	if((id >= 0x8000) 
		|| ((id == IPFIX_TYPEID_flowStartSeconds) && (len == sizeof(unsigned long))) // length of timeval.tv_sec
		|| ((id == IPFIX_TYPEID_flowStartMicroSeconds) && (len ==  sizeof(unsigned long)))
		|| ((id == IPFIX_TYPEID_flowStartMilliSeconds) && (len ==  sizeof(unsigned long long))))
	{
		addFieldWithoutOffset(id, len);
		if (id == IPFIX_TYPEID_flowStartMicroSeconds)
		    msg(MSG_DIALOG, "Warning! flowStartMicroSeconds used as complement for flowStartSeconds (deviating from IPFIX info model).");
        } 
	else 
	{
	    if(getFieldOffsetAndHeader(id, &offset, &header, &validPacketClass) == false)
	    {
		msg(MSG_ERROR, "ID %d currently not supported", id);
		return false;
	    }

	    addFieldWithOffset(id, len, offset, header, validPacketClass);
	}
	
        return true;
}



bool Template::getFieldOffsetAndHeader(uint16_t id, uint16_t *offset, unsigned short *header, unsigned long *validPacketClass)
{
    switch(id)
    {
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
    return true;

}


/* this is not needed within vermont */
#if 0
void AddFieldFromString(Template *temp, const char *field)
{
        if(strncasecmp(field, "SRCIP4", 6) == 0) {
                // source address is as offset 12
                temp->addFieldWithOffset(FT_SRCIP4, 4, 12);
        } else if(strncasecmp(field, "DSTIP4", 6) == 0) {
                // dest address is at offset 16
                temp->addFieldWithOffset(FT_DSTIP4, 4, 16);
        } else if(strncasecmp(field, "PROTO", 5) == 0) {
                // protocol is at offset 9
                temp->addFieldWithOffset(FT_PROTO, 2, 9);
        } else if(strncasecmp(field, "SRCPORT", 7) == 0) {
                // source port is as offset 20 (TCP offset 0)
                temp->addFieldWithOffset(FT_SRCPORT, 2, 20);
        } else if(strncasecmp(field, "DSTPORT", 7) == 0) {
                // dest port is at offset 22 (TCP offset 2)
                temp->addFieldWithOffset(FT_DSTPORT, 2, 22);
        } else if(strncasecmp(field, "NUMPACKETS", 10) == 0) {
		// number of pacets received. This is metadata!
		temp->addFieldWithoutOffset(FT_NUMPACKETS, 4);
        }
}

Template *Template::readFromFile(const char *fileName)
{
        char buffer[256];
        Template *tmp = 0;

        ifstream f(fileName);

        // get template id
        while (!f.eof())
        {
                f.getline(buffer, 255);
                if ((buffer[0] == '#') || (buffer[0] == 0x0d) || (buffer[0] == 0x0a) || (buffer[0] == 0))
                        continue;

                if (strncasecmp(buffer, "ID ", 3) == 0)
                {
                        // assign template id
                        tmp = new Template(strtol(buffer + 3, 0, 10));
                        break;
                }
                else
                {
                        DPRINTF("Expected ID\n");
                        return 0;
                }
        }

        // get template fields
        while (!f.eof())
        {
                f.getline(buffer, 255);
                if ((buffer[0] == '#') || (buffer[0] == 0x0d) || (buffer[0] == 0x0a) || (buffer[0] == 0))
                        continue;

                AddFieldFromString(tmp, buffer);
        }

        f.close();

        return tmp;
}
#endif
