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
#include <sys/types.h>
#include <net/ethernet.h>
#include <sys/time.h>

#include "common/msg.h"
#include "common/defs.h"
#include "common/Mutex.h"
#include "common/ManagedInstance.h"
#include "common/ipfixlolib/encoding.h"

#include <pcap.h>

#include "core/Emitable.h"

// the various header types (actually, HEAD_PAYLOAD is not neccessarily a header but it works like one for
// our purposes)
#define HEAD_RAW                  1
#define HEAD_NETWORK              2  // for fields that lie inside the network header
#define HEAD_NETWORK_AND_BEYOND   3  // for fields that might go beyond the network header border
#define HEAD_TRANSPORT            4  // for fields that lie inside the transport header
#define HEAD_TRANSPORT_AND_BEYOND 5  // for fields that might go beyond the transport header border
#define HEAD_PAYLOAD              6  // field containing TCP/UDP payload (in the case of TCP or UDP) or IP payload (otherwise)


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

// Payload classification
// If known transport protocol is in use, PCLASS_PAYLOAD refers to data beyond transport header.
// Otherwise, PCLASS_PAYLOAD refers to data beyond IP header.
#define PCLASS_PAYLOAD             (1UL << 31)

class Packet :  public ManagedInstance<Packet>, public Emitable
{
public:
	// Transport header classifications (used in Packet::ipProtocolType)
	// Note: ALL is reserved and enables bitoperations using the enums
	enum IPProtocolType { NONE=0x00, TCP=0x01, UDP=0x02, ICMP=0x04, IGMP=0x08, ALL=0xFF };

	// The number of total packets received, will be incremented by each constructor call
	// implemented as public-variable for speed reasons (or lazyness reasons? ;-)
	static unsigned long totalPacketsReceived;

	uint32_t observationDomainID;

	/*
	data: the raw packet data from the wire, including physical header
		The structure contains two fields:
			layer2Field - field containing an layer2 fields of unknown size (unknown at compile time)
			netHeader: start of the IP header. This position is always fixed and does not adopt with varying layer 2 header sizes 
							   (when monitoring a link that contains both tagged and untagged ethernet packets)
							   (This position *MUST* be fixed as it is used as a reference point in the express
							   aggregator). All offsets (e.g. transportLayerOffset, payloadOffset ...) are relative to the
							   start of the network header. ATTENTION: in previous versions of VERMONT, these offsets where
							   relative to the start of the packet header. 
	transportHeader: start of the transport layer header (TCP/UDP): data.netHeader + variable IP header length
	ATTENTION: the data arrays *MUST* be allocated inside the packet structure, so that it has a constant position
	relative to other members of Packet. This is needed for optimization purposes inside the express aggregator
	*/
	unsigned char *layer2Start; // variable pointer that points to the actual start of the layer 2 header
	unsigned int layer2HeaderLen;
	const static unsigned char  maxLayer2HeaderLength = 30;
	struct FullPacketData {
		unsigned char layer2Field[maxLayer2HeaderLength];       // this field contains space for the layer 2 header. Since we might not know 
									// the length of the layer 2 header apriory (ethernet vlans, ...), we reserve
									// a maximum of maxLeayer2HeaderLengthBytes for this field. The real start of the
									// layer 2 header will be recorded in the pointer layer2Start
		unsigned char netHeader[PCAP_MAX_CAPTURE_LENGTH];	// start of the network header
	} DISABLE_ALGINMENT;
	FullPacketData data;
	uint64_t zeroBytes;		/**< needed for reference in fields which are not available in PacketHashtable */
	unsigned char *transportHeader;
	unsigned char *payload;

	// The offsets of the different headers with respect to *fullPacket->netHeader
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
	uint32_t time_sec_nbo, time_usec_nbo; // network byte order, used if exported
	uint64_t time_msec_nbo;   // milliseconds since 1970, according to ipfix standard; ATTENTION: this value is stored in network-byte order

	// buffer for length of variable length fields
	uint8_t varlength[12];
	uint8_t varlength_index;


	Packet(InstanceManager<Packet>* im)
		: ManagedInstance<Packet>(im),
		  zeroBytes(0)
	{
	}

	Packet()
		: ManagedInstance<Packet>(0),
		  zeroBytes(0)
	{
	}

	/**
	 * @param origplen original packet length
	 */
	inline void init(char* packetData, unsigned int len, struct timeval time, uint32_t obsdomainid, uint32_t origplen, int dataLinkType)
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

		layer2HeaderLen = getLayer2HeaderLen(packetData, dataLinkType);
		if (len > PCAP_MAX_CAPTURE_LENGTH || len < layer2HeaderLen) {
			THROWEXCEPTION("received packet of size %d is bigger than maximum length (%d) or smaller than layer 2 len (%d), "
					"adjust compile-time parameter PCAP_MAX_CAPTURE_LENGTH to compensate!", len, PCAP_MAX_CAPTURE_LENGTH, layer2HeaderLen);
		}
		
		// copy all content starting from the IP header
		layer2Start = data.netHeader - layer2HeaderLen;
		memcpy(data.netHeader - layer2HeaderLen , packetData, len);

		// timestamps in network byte order (needed for export or concentrator)
		time_sec_nbo = htonl(timestamp.tv_sec);
		time_usec_nbo = htonl(timestamp.tv_usec);

		// calculate time since 1970 in milliseconds according to IPFIX standard
		time_msec_nbo = htonll(((uint64_t)timestamp.tv_sec * 1000) + (timestamp.tv_usec/1000));
		DPRINTFL(MSG_VDEBUG, "timestamp.tv_sec is %d, timestamp.tv_usec is %d, Human readable: %s", timestamp.tv_sec, timestamp.tv_usec, ctime(&timestamp.tv_sec));
		DPRINTFL(MSG_VDEBUG, "time_msec_ipfix is %llu", time_msec_nbo);

		totalPacketsReceived++;

		classify(dataLinkType);
	};

	inline void init(char** datasegments, uint32_t* segmentlens, struct timeval time, uint32_t obsdomainid, uint32_t origplen, int dataLinkType)
	{
		transportHeader = NULL;
		payload = NULL;
		transportHeaderOffset = 0;
		payloadOffset = 0;
		classification = 0;
		timestamp = time;
		varlength_index = 0;
		ipProtocolType = NONE;
		observationDomainID = obsdomainid;
		pcapPacketLength = origplen;

		data_length = 0;
		layer2HeaderLen = getLayer2HeaderLen(datasegments[0], dataLinkType);
		layer2Start = (data.netHeader - layer2HeaderLen);
		for (uint32_t i=0; datasegments[i]!=0; i++) {
			if (data_length+segmentlens[i] > PCAP_MAX_CAPTURE_LENGTH) {
				THROWEXCEPTION("received packet of size %d is bigger than maximum length (%d), "
					"adjust compile-time parameter PCAP_MAX_CAPTURE_LENGTH to compensate!", data_length+segmentlens[i], PCAP_MAX_CAPTURE_LENGTH);
			}
			memcpy((data.netHeader - layer2HeaderLen)+data_length, datasegments[i], segmentlens[i]);
			data_length += segmentlens[i];
		}

		// timestamps in network byte order (needed for export or concentrator)
		time_sec_nbo = htonl(timestamp.tv_sec);
		time_usec_nbo = htonl(timestamp.tv_usec);

		// calculate time since 1970 in milliseconds according to IPFIX standard
		time_msec_nbo = htonll(((uint64_t)timestamp.tv_sec * 1000) + (timestamp.tv_usec/1000));
		DPRINTFL(MSG_VDEBUG, "timestamp.tv_sec is %d, timestamp.tv_usec is %d", timestamp.tv_sec, timestamp.tv_usec);
		DPRINTFL(MSG_VDEBUG, "time_msec_ipfix is %lld", time_msec_nbo);

		totalPacketsReceived++;

		classify(dataLinkType);
	};

	// Delete the packet and free all data associated with it.
	~Packet()
	{
	}

	unsigned char  getLayer2HeaderLen(const char* packetData, int dataLinkType)
	{
		// get netheader 
		switch (dataLinkType) {
			case DLT_EN10MB: {
				// 14 oder 18
				// check if we have a vlan on the data link layer.
				uint16_t et = ntohs(((struct ether_header*)packetData)->ether_type);
				return 14 + ((et == ETHERTYPE_VLAN)?4:0);
				break;
			}
			case DLT_LOOP:
			case DLT_NULL: {
				return 4;
				break;
			}
			case DLT_LINUX_SLL:{
				return 16;
				break;
			}
			default:
				THROWEXCEPTION("Received packet on not supported link layer \"%u\".", dataLinkType);
				return 0;
		}
		return 0;
	}

	// classify the packet headers
	void classify(int dataLinkType)
	{
		unsigned char protocol = 0;
		uint16_t fragoffset;

		// first check for IPv4 header which needs to be at least 20 bytes long
		if ( (data.netHeader + 20 <= layer2Start + data_length) && ((*data.netHeader >> 4) == 4) )
		{
			protocol = *(data.netHeader + 9);
			classification |= PCLASS_NET_IP4;
			transportHeaderOffset = (( *data.netHeader & 0x0f ) << 2);

			// crop layer 2 padding
			unsigned int endOfIpOffset = layer2HeaderLen +  ntohs(*((uint16_t*) (data.netHeader + 2)));
			if(data_length > endOfIpOffset)
			{
				DPRINTF("crop layer 2 padding: old: %u  new: %u\n", data_length, endOfIpOffset);
				data_length = endOfIpOffset;
			}

			// get fragment offset
			fragoffset = (*(uint16_t*)(data.netHeader+6))&0xFF1F;

			// do not use transport header, if this is not the first fragment
			// in the end, all fragments are discarded by vermont (TODO!)
			if(transportHeaderOffset < data_length && fragoffset==0)
				transportHeader = data.netHeader + transportHeaderOffset;
			else
				transportHeaderOffset = 0;
		}

		// check for IPv6 header, fixed header is 40 bytes long
		else if ( (data.netHeader + 40 <= layer2Start + data_length) && ((*data.netHeader >> 4) == 6) )
		{
			protocol = *(data.netHeader + 7);
			classification |= PCLASS_NET_IP6;
			transportHeaderOffset = 40;

			bool extHeaderPresent = true;
			while (extHeaderPresent) {
				switch (protocol) {
					case 0:		// Hop-by-Hop Options
					case 60:	// Destination Options
					case 43:	// Routing
					case 135:	// Mobility
						protocol = *(data.netHeader + transportHeaderOffset);
						// length of header is multiple of 8 octets, not considering the first eight octets
						transportHeaderOffset += ((*(data.netHeader + transportHeaderOffset + 1)) << 3) + 8;
						break;

					case 44:	// Fragment
						// Only use transport header if this is the first fragment
						fragoffset = ((*((uint16_t*) (data.netHeader + transportHeaderOffset + 2)))) & 0xF8FF;
						if (fragoffset == 0) {
							protocol = *(data.netHeader + transportHeaderOffset);
							transportHeaderOffset += 8;
						} else {
							transportHeaderOffset = 0;
							extHeaderPresent = false;
						}
						break;

					case 51:	// Authentication Header
						protocol = *(data.netHeader + transportHeaderOffset);
						// length of header is stored as multiple of 4 octets minus 2 octets
						transportHeaderOffset += ((*(data.netHeader + transportHeaderOffset + 1) + 2) << 2);
						break;

					case 50:	// Encapsulating Security Payload, length and next header are encrypted
					default:	// No (more) extension header present
						extHeaderPresent = false;
				}

			}

			// crop layer 2 padding
			unsigned int endOfIpOffset = layer2HeaderLen +  ntohs(*((uint16_t*) (data.netHeader + 2)));
			if(data_length > endOfIpOffset)
			{
				DPRINTF("crop layer 2 padding: old: %u  new: %u\n", data_length, endOfIpOffset);
				data_length = endOfIpOffset;
			}

			// Set transport header
			transportHeader = data.netHeader + transportHeaderOffset;
		}

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
						unsigned char tcdataOffset = *(transportHeader + 12) >> 4;

						// calculate payload offset
						payloadOffset = transportHeaderOffset + (tcdataOffset << 2);

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
				payload = data.netHeader + payloadOffset;
			}
			else
				// there is no payload
				payloadOffset = 0;
		}

		DPRINTFL(MSG_VDEBUG, "Packet::classify: class %08lx, proto %d, data %p, net %p, trn %p, payload %p\n", classification, protocol, data, data.netHeader, transportHeader, payload);
	}

	// read data from the IP header
	void copyPacketData(void *dest, int offset, int size) const
	{
		memcpy(dest, (char *)data.netHeader + offset, size);
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
	void * getPacketData(uint16_t offset, uint16_t header, uint16_t fieldLength) const
	{
	    DPRINTF("offset: %d header: %d fieldlen: %d available: %d\n", offset, header, fieldLength, data_length);
	    switch(header)
	    {

		// for the following types, we omit the length check
		case HEAD_NETWORK:
		    return (void*)data.netHeader;
		case HEAD_TRANSPORT:
		    return transportHeader + offset;

		// the following types may be variable length
		// if not, we have to check that the length is not too long
		case HEAD_RAW:
		    return ((unsigned int)offset + fieldLength <= data_length) ? (char*)layer2Start + offset : NULL;
		case HEAD_NETWORK_AND_BEYOND:
		    return (offset + fieldLength <= data_length) ? (char*)data.netHeader + offset : NULL;
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
	void * getVariableLengthPacketData(uint16_t *length, uint8_t **enc_value, uint8_t *enc_len, uint16_t offset, uint16_t header)
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
		    packetdata = layer2Start + offset;
		    break;

		case HEAD_NETWORK:
		    // return only data inside network header
		    if(transportHeader == NULL)
			len = data_length - offset;
		    else
			len = transportHeaderOffset - offset;
		    packetdata = data.netHeader + offset;
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
		    len = data_length - offset;
		    packetdata = data.netHeader + offset;
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



};

#endif
