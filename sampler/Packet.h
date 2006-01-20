/*
 * PSAMP Reference Implementation
 *
 * Packet.h
 *
 * Encapsulates a captured packet with simple, thread-aware
 * reference-(usage-) counting.
 *
 * Author: Michael Drueing <michael@drueing.de>
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

#include "msg.h"

#include "Lock.h"

// the various header types (actually, HEAD_PAYLOAD is not neccessarily a header but it works like one for
// our purposes)
#define HEAD_RAW        0
#define HEAD_NETWORK    1
#define HEAD_TRANSPORT  2
#define HEAD_PAYLOAD    3

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
#define PCLASS_NET_IV6             (1UL <<  1)

#define PCLASS_NETMASK             0x00000fff

// Transport header classifications
#define PCLASS_TRN_TCP             (1UL << 12)
#define PCLASS_TRN_UDP             (1UL << 13)
#define PCLASS_TRN_ICMP            (1UL << 14)
#define PCLASS_TRN_IGMP            (1UL << 15)

#define PCLASS_TRNMASK             0x00fff000
// .... etc. etc. etc. 

// Payload classification
#define PCLASS_PAYLOAD             (1UL << 31)

class Packet
{
public:
	// The number of total packets received, will be incremented by each constructor call
	// implemented as public-variable for speed reasons (or lazyness reasons? ;-)
	static unsigned long totalPacketsReceived;

	// this buffer is returned for all requested data that is out-of-bounds for the packet,
	// like data after the physical end, header fields of invalid (short) TCP packets, etc.
	static unsigned char nullBuffer[64];
	
	/*
	 data: the raw packet data from the wire, including physical header
	 ipHeader: start of the IP header: data + (physical dependent) IP header offset
	 transportHeader: start of the transport layer header (TCP/UDP): ip_header + variable IP header length
	 */
	unsigned char *data;
	unsigned char *netHeader;
	unsigned char *transportHeader;
	unsigned char *payload;

	// the packet classification, i.e. what headers are present?
	unsigned long classification;

	// The length of the packet in bytes
	unsigned int length;

	// when was the packet received?
	struct timeval timestamp;

	// construct a new Packet for a specified number of 'users'
	Packet(void *packetData, unsigned int len, int numUsers = 1) : users(numUsers), refCountLock()
	{
		data = (unsigned char *)packetData;
		netHeader = data + IPHeaderOffset;
		//transportHeader = (unsigned char *)netHeader + netTransportHeaderOffset(netHeader);
		length = len;

		totalPacketsReceived++;

		classify();
		/*
		 DO NOT SET TIMESTAMP HERE
		 IS SET IN OBSERVER!
		 */
	};

	// Delete the packet and free all data associated with it. Should only be called
	// if users==0 !
	~Packet()
	{
		if(users > 0) {
			DPRINTF("Packet: WARNING: freeing in-use packet!\n");
		}

		free(data);
	}

	// call this function after processing the packet, NOT delete()!
	void release()
	{
		int newUsers;

		refCountLock.lock();
		--users;
		newUsers = users;
		refCountLock.unlock();

		if(newUsers == 0) {
			delete this;
		} else if(newUsers < 0) {
			DPRINTF("Packet: WARNING: trying to free already freed packet!\n");
		}
	};

	// classify the packet headers
	void classify()
	{
		unsigned char protocol = 0;
		unsigned char tcpDataOffset;
		
		classification = 0;
		transportHeader = 0;
		payload = 0;
		
		// first check for IPv4 header
		if ( (*netHeader >> 4) == 4)
		{
			classification |= PCLASS_NET_IP4;
			transportHeader = netHeader + ( ( *netHeader & 0x0f ) << 2);
			protocol = *(netHeader + 9);
		}

		// if we found a transport header, continue classifying
		if (transportHeader && protocol)
		{
			switch (protocol)
			{
			case 1:		// ICMP
				classification |= PCLASS_TRN_ICMP;

				// ICMP header is 4 bytes fixed-length
				payload = transportHeader + 4;
				
				break;
			case 2:		// IGMP
				classification |= PCLASS_TRN_IGMP;

				// header is 8-bytes fixed size
				payload = transportHeader + 8;
				
				break;
			case 6:         // TCP
				classification |= PCLASS_TRN_TCP;
				
				// extract "Data Offset" field at TCP header offset 12 (upper 4 bits)
				tcpDataOffset = *(transportHeader + 12) >> 4;
				
				// calculate payload offset
				payload = transportHeader + (tcpDataOffset << 2);
				
				break;
				
			case 17:        // UDP
				classification |= PCLASS_TRN_UDP;
				// UDP has a fixed header size of 8 bytes

				payload = transportHeader + 8;

				break;
				
			default:
				break;
			}

			// check if we actually _have_ payload
			if (payload && (payload < data + length))
				classification |= PCLASS_PAYLOAD;

			//fprintf(stderr, "proto %d, data %p, net %p, trn %p, payload %p\n", protocol, data, netHeader, transportHeader, data);
		}
	}

	// read data from the IP header
	void copyPacketData(void *dest, int offset, int size) const
	{
		memcpy(dest, (char *)netHeader + offset, size);
	}

	// return a pointer into the packet to IP header offset given
	void * getPacketData(int offset, int header) const
	{
		unsigned char *dataOffset = 0;
		
		switch(header)
		{
		case HEAD_RAW:
			dataOffset = data + offset;
		case HEAD_NETWORK:
			dataOffset = netHeader + offset;
		case HEAD_TRANSPORT:
			dataOffset = transportHeader + offset;
// TODO: need to check with packet length, too			
//		case HEAD_PAYLOAD:
//			dataOffset = payload + offset;
		default:
			dataOffset = data + offset;
		}

		return (dataOffset < data + length) ? dataOffset : nullBuffer;
	}

private:
	/*
	 the raw offset at which the IP header starts in the packet
	 for Ethernet, this is 14 bytes (MAC header size)
	 */
	static const int IPHeaderOffset=14;

	/*
	 Number of concurrent users of this packet. Decremented each time
	 release() is called. After it reaches zero, the packet is deleted.
         */
	int users;

	Lock refCountLock;

	/*
	 return the offset the transport header lies
	 IP knows about variable length options field
         */
	static inline unsigned int netTransportHeaderOffset(void *ipPacket)
	{
		/*
		 the header length (incl. options field) is:
		 last 4 bits in the first byte * 4bytes
		 */
		unsigned char len = *((unsigned char *)ipPacket) & 0x0f;

		return len << 2;
	}

};

#endif
