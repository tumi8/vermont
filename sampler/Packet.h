/*
 * PSAMP Reference Implementation
 *
 * Packet.h
 *
 * Encapsulates a captured packet with simple, thread-aware
 * reference-(usage-) counting.
 *
 * Author: Michael Drueing <michael@drueing.de>
 *         Gerhard Muenz <gerhard.muenz@gmx.de>
 *
 */

/*
 changed by: Ronny T. Lampert, 2005, for VERMONT
 */

#ifndef PACKET_H
#define PACKET_H

#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <cstring>
#include <stdint.h>
#include <netinet/in.h>
#include <sys/time.h>

#include "common/msg.h"
#include "common/defs.h"
#include "common/Mutex.h"
#include "common/ManagedInstance.h"
#include "ipfixlolib/encoding.h"

#include "reconf/Emitable.h"

// the various header types (actually, HEAD_PAYLOAD is not neccessarily a header but it works like one for
// our purposes)
#define HEAD_RAW                  1
#define HEAD_NETWORK              2  // for fields that lie inside the network header
#define HEAD_NETWORK_AND_BEYOND   3  // for fields that might go beyond the network header border
#define HEAD_TRANSPORT            4  // for fields that lie inside the transport header
#define HEAD_TRANSPORT_AND_BEYOND 5  // for fields that might go beyond the transport header border
#define HEAD_PAYLOAD              6


// Packet classifications
//
// This bitmask defines various classifications for the packet. Which protocol headers it contains for example
// Bits:
//   3                   2                   1
// 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
// |               |~~~~~~~~~~~~~~~~~~~~~| |~~~~~~~~~~~~~~~~~~~~~|
// |               |                     | +---------------------+----- Network headers
// |               +---------------------+----------------------------- Transport headers
// +------------------------------------------------------------------- Payload available

// Network header classifications
#define PCLASS_NET_IP4             (1UL <<  0)
#define PCLASS_NET_IP6             (1UL <<  1)

#define PCLASS_NETMASK             0x00000fff

// Transport header classifications
#define PCLASS_TRN_TCP             (1UL << 12)
#define PCLASS_TRN_UDP             (1UL << 13)
#define PCLASS_TRN_ICMP            (1UL << 14)
#define PCLASS_TRN_IGMP            (1UL << 15)

#define PCLASS_TRNMASK             0x00fff000
// .... etc. etc. etc.

// Payload classification (here, payload refers to data beyond transport header)
#define PCLASS_PAYLOAD             (1UL << 31)

class Packet :  public ManagedInstance<Packet>, public Emitable
{
public:
	/*
	 the raw offset at which the IP header starts in the packet
	 for Ethernet, this is 14 bytes (MAC header size).
	 This constant is set via the configure script. It defaults to 14
	 */
	static const int IPHeaderOffset=IP_HEADER_OFFSET;

	// Transport header classifications (used in Packet::ipProtocolType)
	// Note: ALL is reserved and enables bitoperations using the enums
	enum IPProtocolType { NONE=0x00, TCP=0x01, UDP=0x02, ICMP=0x04, IGMP=0x08, ALL=0xFF };

	// The number of total packets received, will be incremented by each constructor call
	// implemented as public-variable for speed reasons (or lazyness reasons? ;-)
	static unsigned long totalPacketsReceived;

	uint32_t observationDomainID;

	/*
	data: the raw packet data from the wire, including physical header
	ipHeader: start of the IP header: data + (physical dependent) IP header offset
	transportHeader: start of the transport layer header (TCP/UDP): ip_header + variable IP header length
	ATTENTION: this array *MUST* be allocated inside the packet structure, so that it has a constant position
	relative to other members of Packet. This is needed for optimization purposes inside the express aggregator
	*/
	unsigned char data[PCAP_MAX_CAPTURE_LENGTH];
	uint64_t zeroBytes;		/**< needed for reference in fields which are not available in PacketHashtable */
	unsigned char *netHeader;
	unsigned char *transportHeader;
	unsigned char *payload;

	// The offsets of the different headers with respect to *data
	unsigned int netHeaderOffset;
	unsigned int transportHeaderOffset;
	unsigned int payloadOffset;

	// the packet classification, i.e. what headers are present?
	// note: protocol type is also specified in ipProtocolType
	unsigned long classification;

	// note: protocol type is also specified in classification
	IPProtocolType ipProtocolType;

	// The number of captured bytes
	unsigned int data_length;
	// original packet length
	uint32_t pcapPacketLength;

	// when was the packet received?
	struct timeval timestamp;
	unsigned long time_sec_nbo, time_usec_nbo; // network byte order, used if exported
	unsigned long long time_msec_nbo;   // milliseconds since 1970, according to ipfix standard; ATTENTION: this value is stored in network-byte order

	// buffer for length of variable length fields
	uint8_t varlength[12];
	uint8_t varlength_index;


	Packet(InstanceManager<Packet>* im)
		: ManagedInstance<Packet>(im),
		  zeroBytes(0),
		  netHeader(data + IPHeaderOffset), // netHeader must not be changed afterwards
		  netHeaderOffset(IPHeaderOffset)
	{
	}

	Packet()
		: ManagedInstance<Packet>(0),
		  netHeader(data + IPHeaderOffset),
		  netHeaderOffset(IPHeaderOffset)
	{
	}

	/**
	 * @param origplen original packet length
	 */
	inline void init(char* packetData, int len, struct timeval time, uint32_t obsdomainid, uint32_t origplen)
	{
		transportHeader = NULL;
		payload = NULL;
		transportHeaderOffset = 0;
		payloadOffset = 0;
		classification = 0;
		data_length = len;
		timestamp = time;
		varlength_index = 0;
		ipProtocolType = NONE;
		observationDomainID = obsdomainid;
		pcapPacketLength = origplen;

		if (len > PCAP_MAX_CAPTURE_LENGTH) {
			THROWEXCEPTION("received packet of size %d is bigger than maximum length (%d), "
					"adjust compile-time parameter PCAP_MAX_CAPTURE_LENGTH to compensate!", len, PCAP_MAX_CAPTURE_LENGTH);
		}

		memcpy(data, packetData, len);

		// timestamps in network byte order (needed for export or concentrator)
		time_sec_nbo = htonl(timestamp.tv_sec);
		time_usec_nbo = htonl(timestamp.tv_usec);

		// calculate time since 1970 in milliseconds according to IPFIX standard
		time_msec_nbo = htonll(((unsigned long long)timestamp.tv_sec * 1000) + (timestamp.tv_usec/1000));
		DPRINTFL(MSG_VDEBUG, "timestamp.tv_sec is %d, timestamp.tv_usec is %d", timestamp.tv_sec, timestamp.tv_usec);
		DPRINTFL(MSG_VDEBUG, "time_msec_ipfix is %lld", time_msec_nbo);

		totalPacketsReceived++;

		classify();
	};

	// Delete the packet and free all data associated with it. Should only be called
	// if users==0 !
	~Packet()
	{
	}

	// classify the packet headers
	void classify()
	{
		unsigned char protocol = 0;

		// first check for IPv4 header which needs to be at least 20 bytes long
		if ( (netHeader + 20 <= data + data_length) && ((*netHeader >> 4) == 4) )
		{
			protocol = *(netHeader + 9);
			classification |= PCLASS_NET_IP4;
			transportHeaderOffset = netHeaderOffset + (( *netHeader & 0x0f ) << 2);

			// crop layer 2 padding
			unsigned int endOfIpOffset = netHeaderOffset + ntohs(*((uint16_t*) (netHeader + 2)));
			if(data_length > endOfIpOffset)
			{
				DPRINTF("crop layer 2 padding: old: %u  new: %u\n", data_length, endOfIpOffset);
				data_length = endOfIpOffset;
			}

			// get fragment offset
			uint16_t fragoffset = (*(uint16_t*)(netHeader+6))&0xFF1F;

			// do not use transport header, if this is not the first fragment
			// in the end, all fragments are discarded by vermont (TODO!)
			if(transportHeaderOffset < data_length && fragoffset==0)
				transportHeader = data + transportHeaderOffset;
			else
				transportHeaderOffset = 0;
		}
		// TODO: Add checks for IPv6 or similar here

		// if we found a transport header, continue classifying
		if (transportHeader && protocol)
		{
			switch (protocol)
			{
				case 1:		// ICMP
					// ICMP header is 4 bytes fixed-length
					payloadOffset = transportHeaderOffset + 4;

					// check if the packet is big enough to actually be ICMP
					if (payloadOffset <= data_length) {
						ipProtocolType = ICMP;
						classification |= PCLASS_TRN_ICMP;
					} else {
						// there is no complete transport heaader => treat data it as payload
						payloadOffset = transportHeaderOffset;
					}
					break;
				case 2:		// IGMP
					// header is 8-bytes fixed size
					payloadOffset = transportHeaderOffset + 8;

					// check if the packet is big enough to actually be IGMP
					if (payloadOffset <= data_length) {
						ipProtocolType = IGMP;
						classification |= PCLASS_TRN_IGMP;
					} else {
						// there is no complete transport heaader => treat data it as payload
						payloadOffset = transportHeaderOffset;
					}
					break;
				case 6:         // TCP
					// we need at least 12 more bytes in the packet to extract the "Data Offset"
					if (transportHeaderOffset + 12 < data_length)
					{
						// extract "Data Offset" field at TCP header offset 12 (upper 4 bits)
						unsigned char tcpDataOffset = *(transportHeader + 12) >> 4;

						// calculate payload offset
						payloadOffset = transportHeaderOffset + (tcpDataOffset << 2);

						// check if the complete TCP header is inside the received packet data
						if (payloadOffset <= data_length) {
							ipProtocolType = TCP;
							classification |= PCLASS_TRN_TCP;
						} else {
							// there is no complete transport heaader => treat data it as payload
							payloadOffset = transportHeaderOffset;
						}
					}
					else
						// there is no complete transport heaader => treat data it as payload
						payloadOffset = transportHeaderOffset;

					break;
				case 17:        // UDP
					// UDP has a fixed header size of 8 bytes
					payloadOffset = transportHeaderOffset + 8;

					// check if the packet is big enough to actually be UDP
					if (payloadOffset <= data_length) {
						ipProtocolType = UDP;
						classification |= PCLASS_TRN_UDP;
					} else {
						// there is no complete transport heaader => treat data it as payload
						payloadOffset = transportHeaderOffset;
					}
					break;
				default:	// unknown transport protocol or insufficient data length
					// omit transport header and classify it as payload
					payloadOffset = transportHeaderOffset;
					break;
			}

			// check if we actually _have_ payload
			if ((payloadOffset > 0) && (payloadOffset < data_length))
			{
				classification |= PCLASS_PAYLOAD;
				payload = data + payloadOffset;
			}
			else
				// there is no payload
				payloadOffset = 0;
		}

		DPRINTFL(MSG_VDEBUG, "Packet::classify: class %08lx, proto %d, data %p, net %p, trn %p, payload %p\n", classification, protocol, data, netHeader, transportHeader, payload);
	}

	// read data from the IP header
	void copyPacketData(void *dest, int offset, int size) const
	{
		memcpy(dest, (char *)netHeader + offset, size);
	}


	// return a pointer into the packet to IP header offset given
	//
	// ATTENTION: If the returned pointer actually points to data of size fieldLength is only
	// checked for HEAD_RAW, HEAD_NETWORK_AND_BEYOND, HEAD_TRANSPORT_AND_BEYOND, and HEAD_PAYLOAD.
	// For fields within the network or transport header (HEAD_NETWORK, HEAD_TRANSPORT), we assume
	// that it was verified before that the packet is of the correct packet class and that its buffer
	// holds enough data.
	// You can check the packet class for a single field using match(). If you want to check
	// against all fields in a template, you should use checkPacketConformity() of the Template class.
	// If enough data for the network/transport header has been captured, is checked by classify().
	void * getPacketData(unsigned short offset, unsigned short header, unsigned short fieldLength) const
	{
	    DPRINTF("offset: %d header: %d fieldlen: %d available: %d\n", offset, header, fieldLength, data_length);
	    switch(header)
	    {

		// for the following types, we omit the length check
		case HEAD_NETWORK:
		    return netHeader + offset;
		case HEAD_TRANSPORT:
		    return transportHeader + offset;

		// the following types may be variable length
		// if not, we have to check that the length is not too long
		case HEAD_RAW:
		    return ((unsigned int)offset + fieldLength <= data_length) ? (char*)data + offset : NULL;
		case HEAD_NETWORK_AND_BEYOND:
		    return (netHeaderOffset + offset + fieldLength <= data_length) ? netHeader + offset : NULL;
		case HEAD_TRANSPORT_AND_BEYOND:
		    return (transportHeaderOffset + offset + fieldLength <= data_length) ? transportHeader + offset : NULL;
		case HEAD_PAYLOAD:
		    return (payloadOffset + offset + fieldLength <= data_length) ? payload + offset : NULL;

		// this should never be reached...
		default:
		    return NULL;
	    }
	}

	// returns the pointer packetdata within the packet at position (header, offset)
        // determines the encoded available data length and the length of the encoded length value in octets
	// (=1 if length < 255, =3 if 255 <= length <= 65535)
	// cf. IPFIX protocol draft
	void * getVariableLengthPacketData(unsigned short *length, uint8_t **enc_value, unsigned short *enc_len, int offset, int header)
	{
	    int len;
	    void *packetdata = NULL;

	    // check if we have enough space to buffer at least one octet
	    if(!(varlength_index < sizeof(varlength)))
	    {
		msg(MSG_ERROR, "getVariableLengthPacketData: varlength[] is too small");
		return NULL;
	    }

	    switch(header)
	    {
		case HEAD_RAW:
		    len = data_length - offset;
		    packetdata = data + offset;
		    break;

		case HEAD_NETWORK:
		    // return only data inside network header
		    if(transportHeader == NULL)
			len = data_length - netHeaderOffset - offset;
		    else
			len = transportHeaderOffset - netHeaderOffset - offset;
		    packetdata = netHeader + offset;
		    break;

		case HEAD_TRANSPORT:
		    // return only data inside transport header
		    if(payload == NULL)
			len = data_length - transportHeaderOffset - offset;
		    else
			len = payloadOffset - transportHeaderOffset - offset;
		    packetdata = transportHeader + offset;
		    break;

		case HEAD_NETWORK_AND_BEYOND:
		    len = data_length - netHeaderOffset - offset;
		    packetdata = netHeader + offset;
		    break;

		case HEAD_TRANSPORT_AND_BEYOND:
		    len = data_length - transportHeaderOffset - offset;
		    packetdata = transportHeader + offset;
		    break;

		case HEAD_PAYLOAD:
		    len = data_length - payloadOffset - offset;
		    packetdata = payload + offset;
		    break;

		// this should never be reached...
		default:
		    return NULL;
	    }

	    // if len is negative, the offset points outside the available data
	    if(len <= 0)
		return NULL;

	    *length = (unsigned short) len;

	    // encode length
	    if(*length < 255)
	    {
		// encode in 1 octet
		varlength[varlength_index] = (uint8_t)(*length);
		*enc_value = &(varlength[varlength_index]);
		*enc_len = 1;
		varlength_index++;
	    }
	    // check if we have 3 octets available
	    else if ((uint8_t)(varlength_index + 3) <= sizeof(varlength))
	    {
		// encode in 2 octets with preceeding 255
		varlength[varlength_index] = 255;
		//varlength[varlength_index + 1] = (uint8_t) (*length);
		//varlength[varlength_index + 2] = (uint8_t) ((*length) >> 8);
		*(uint16_t*)(varlength + varlength_index + 1) = htons((uint16_t) (*length));
		*enc_value = &(varlength[varlength_index]);
		*enc_len = 3;
		varlength_index = varlength_index + 3;
	    }
	    else
	    {
		msg(MSG_ERROR, "getVariableLengthPacketData: varlength[] is too small");
		return NULL;
	    }

	    //msg(MSG_INFO, "offset: %d header: %d fieldlen: %d available: %d, encoded: %d-%d-%d, %d", offset, header, len, data_length, (uint8_t)(**enc_value), (uint8_t)(*(*enc_value+1)), (uint8_t)(*(*enc_value+2)), *enc_len);

	    return packetdata;
	}



private:

	/*
	 Number of concurrent users of this packet. Decremented each time
	 release() is called. After it reaches zero, the packet is deleted.
         */
	int users;

	Mutex refCountLock;

	bool packetFreed;

};

#endif
