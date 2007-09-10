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

#include "reconf/Destination.h"

#include <string>

using namespace std;


class PacketReceiver : public StatisticsModule, public Destination<Packet*>
{
private:
	ConcurrentQueue<Packet*>  queue;
	string					  name;

public:
	PacketReceiver(string ownerName);

	virtual ~PacketReceiver();

	virtual void receive(Packet* p)
	{
		msg(MSG_ERROR, "PacketReceiver::receive(Packet*) called\n");
		queue.push(p);
	}
	
	inline virtual ConcurrentQueue<Packet*> *getQueue() const
	{
		return const_cast<ConcurrentQueue<Packet*>*>(&queue);
	}

	virtual std::string getStatistics();
};

#endif
