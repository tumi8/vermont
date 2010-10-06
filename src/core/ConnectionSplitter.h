#ifndef PACKETCONNECTIONSPLICER_H
#define PACKETCONNECTIONSPLICER_H

/**
	@author Peter Baumann <siprbaum@users.berlios.de>
*/

#include "common/msg.h"

#include "core/Adapter.h"
#include "core/Destination.h"
#include "core/Source.h"

#include "modules/packet/Packet.h"

#include <vector>

template <class T>
class ConnectionSplitter
	: public Adapter<T>
{
public:
	ConnectionSplitter(): size(0) { }

	virtual ~ConnectionSplitter() { }

	virtual void connectTo(Destination<T>* d)
	{
		Source<T>::mutex.lock();

		if (size >= capacity)
			THROWEXCEPTION("ConnectionSplitter: can't add another destination\n");

		destinations[size++] = d;
		Source<T>::connected.inc(1);
		
		Source<T>::mutex.unlock();

	}

	virtual bool isConnected() const
	{
		return size > 0;
	}

	virtual void disconnect()
	{
		THROWEXCEPTION("Can't disconnect from a splicer\n");
	}

	virtual void receive(T packet, int id)
	{
		process(packet, id);
	}
	
	virtual void receive(T packet)
	{
		// this is in an own function to later have the possibility to process
		// the packets in an own thread; for now, I think this is not needed
		process(packet);
	}
	virtual void sendQueueRunningNotification() {
		for (size_t i = 0; i < size; i++) {
			destinations[i]->notifyQueueRunning();
		}
	}

private:
	inline void process(T packet, int id = -1)
	{
		if (!Source<T>::sleepUntilConnected()) {
			DPRINTF("Can't wait for connection, perhaps the program is shutting down?");
			return;
		}
		
		/* By processing the array over a mutex proctected variable size
		 * it should be safe to add another destination.
		 * In the worst case, we don't push the packet to a newly added destination.
		 */
		size_t sz = size;
		if (sz > 1) // using this strange construct because size-1 could wrap
			packet->addReference(sz - 1);

		size_t i;
		for (i = 0; i < sz; i++) {
			if(id == -1) {
				//packet shall be forwarded to every successor
				destinations[i]->receive(packet);
				continue;
			}
			if(id == i){
				//forward packet to only one succeeding module
				destinations[i]->receive(packet);
				break;
			}
		}
	}

	static const size_t capacity = 10;
	Destination<T>* destinations[capacity];
	size_t size;
};

#endif

