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

#include "common/ConcurrentQueue.h"
#include "common/StatisticsManager.h"
#include "Packet.h"

#include <string>

class PacketReceiver : public StatisticsModule
{
private:
	ConcurrentQueue<Packet*>  queue;
	std::string name;

public:
	PacketReceiver(const std::string& ownerName);

	virtual ~PacketReceiver();

	inline virtual ConcurrentQueue<Packet*> *getQueue() const
	{
		return const_cast<ConcurrentQueue<Packet*>*>(&queue);
	}

	virtual std::string getStatistics();
};

#endif
