#ifndef CONNECTIONQUEUE_H
#define CONNECTIONQUEUE_H

/**
	@author Peter Baumann <siprbaum@users.berlios.de>
*/

#include "reconf/Destination.h"
#include "reconf/Source.h"

#include "common/ConcurrentQueue.h"
#include "sampler/Packet.h"

template <class T>
class ConnectionQueue
	: public Source<T>, public Destination<T>
{
public:
	ConnectionQueue() : queue() { }
	virtual ~ConnectionQueue() { }

	virtual ~ConnectionQueue();

	virtual void receive(T* packet)
	{
		printf("receive(Packet*)\n");
		queue.push(packet);

		// FIXME: this should be in an own thread
		process();
	}

private:
	void process()
	{
		T* packet = NULL;
		Destination<T>* d;

		/* FIXME: synchronisation handling if this is called in an own thread;
		 *	  especially if one is allowed to disconnect the from the queue
		 */
		while ((d = dest) != NULL) {
			queue.pop(&packet);
			/* this should work if dest is disconnected between the while() and the actual call
			 * of receive() IF dest isn't freed already
			 */
			d->receive(packet);
		}
	}

	ConcurrentQueue<T*> queue;
};

#endif

