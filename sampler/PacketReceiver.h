/*
 * PSAMP Reference Implementation
 *
 * PacketReceiver.h
 *
 * Base class for all Packet-receiving classes
 *
 * Author: Michael Drueing <michael@drueing.de>
 *
 */

#ifndef PACKET_RECEIVER_H
#define PACKET_RECEIVER_H

#include "ConcurrentQueue.h"
#include "Packet.h"

class PacketReceiver
{
protected:
	ConcurrentQueue<Packet> *queue;

public:
	PacketReceiver()
	{
		queue = new ConcurrentQueue<Packet>();
	}

	virtual ~PacketReceiver()
	{
		delete queue;
	}

	inline virtual ConcurrentQueue<Packet> *getQueue() const
	{
		return queue;
	}

};

#endif
