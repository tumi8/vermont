#ifndef PACKETCONNECTIONSPLICER_H
#define PACKETCONNECTIONSPLICER_H

/**
	@author Peter Baumann <siprbaum@users.berlios.de>
*/

#include "reconf/Destination.h"
#include "reconf/Source.h"

#include "sampler/Packet.h"

#include <vector>

template <class T>
class ConnectionSplicer
	: public Source<T>, public Destination<T>
{
public:
	ConnectionSplicer(): size(0) { }

	virtual ~ConnectionSplicer() { }

	virtual void connectTo(Destination<T*> d)
	{
		Source<T>::mutex.lock();

		if (size >= capacity)
			throw std::runtime_error("ConnectionSplicer: can't add another destination\n");

		destinations[size++] = d;

		Source<T>::mutex.unlock();
	}

	virtual bool isConnected() const
	{
		return size > 0;
	}

	virtual void receive(T* packet)
	{
		printf("PacketConnectionSplicer::receive(Packet*)\n");

		// this is in an own function to later have the possibility to process
		// the packets in an own thread; for now, I think this is not needed
		process(packet);
	}

private:
	inline void process(T *packet)
	{
		/* By processing the array over a mutex proctected variable size
		 * it should be safe to add another destination.
		 * In the worst case, we don't push the packet to a newly added destination.
		 */
		size_t sz = size;
		if (sz > 1) // using this strange construct because size-1 could wrap
			packet->addReference(sz - 1);

		size_t i;
		for (i = 0; i < sz; i++) {
			destinations[i]->receive(packet);
		}
	}

	static const size_t capacity = 10;
	Destination<T>* destinations[capacity];
	size_t size;
};

ConnectionSplicer<Packet> a;

#endif

