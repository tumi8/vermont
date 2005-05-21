/*
 * PSAMP Reference Implementation
 *
 * PacketProcessor.h
 *
 * Base class for a packet processor (filter/sampler)
 *
 * Author: Michael Drueing <michael@drueing.de>
 *
 */

#ifndef PACKET_PROCESSOR_H
#define PACKET_PROCESSOR_H

#include "Packet.h"

class PacketProcessor
{
public:
	/*
	 processPacket(Packet *)
	 processes a packet and returns true if the packet passes the
	 filter/sampler and false if it should be dropped
	*/
	virtual bool processPacket(const Packet *p) = 0;
};

#endif
