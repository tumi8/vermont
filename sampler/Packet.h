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

class Packet
{
public:
	/*
	 data: the raw packet data from the wire, including physical header
	 ipHeader: start of the IP header: data + (physical dependent) IP header offset
	 transportHeader: start of the transport layer header (TCP/UDP): ip_header + variable IP header length
	 */
	void *data;
	void *ipHeader;
	void *transportHeader;

	// The length of the packet in bytes
	unsigned int length;

	// when was the packet received?
	struct timeval timestamp;

	// construct a new Packet for a specified number of 'users'
	Packet(void *packetData, unsigned int len, int numUsers = 1) : users(numUsers), refCountLock()
	{
		data = packetData;
		ipHeader = (unsigned char *)data + IPHeaderOffset;
		transportHeader = (unsigned char *)ipHeader + ipTransportHeaderOffset(ipHeader);
		length = len;

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

		if(newUsers < 0) {
			DPRINTF("Packet: WARNING: trying to free already freed packet!\n");
		}

		if(newUsers == 0) {
			delete this;
		}
	};

	// read data from the IP header
	void copyPacketData(void *dest, int offset, int size) const
	{
		memcpy(dest, (char *)ipHeader + offset, size);
	}

	// return a pointer into the packet to IP header offset given
	void * getPacketData(int offset) const
	{
		return (char *)ipHeader + offset;
	}

private:
	/*
	 the raw offset at which the IP header starts in the packet
	 for Ethernet, this is 14 bytes (MAC header size)
	 */
	static int IPHeaderOffset;

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
	static inline unsigned int ipTransportHeaderOffset(void *ipPacket)
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
