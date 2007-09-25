#ifndef CONNECTIONQUEUE_H
#define CONNECTIONQUEUE_H

/**
	@author Peter Baumann <siprbaum@users.berlios.de>
*/

#include "Adapter.h"
#include "Destination.h"
#include "Module.h"
#include "Source.h"
#include "Timer.h"

#include "common/ConcurrentQueue.h"
#include "common/msg.h"
#include "common/Thread.h"
#include "sampler/Packet.h"

#include <list>


using namespace std;


/**
 * used to manage timeout entries inside ConnectionQueue
 */
struct TimeoutEntry 
{
	Notifiable* n;
	struct timespec timeout;
	uint32_t flag;
};


template <class T>
class ConnectionQueue : public Adapter<T>, public Timer
{
public:
	ConnectionQueue(uint32_t maxEntries = 1)
		: queue(maxEntries), thread(threadWrapper)
	{ 
	}

	virtual ~ConnectionQueue()
	{
	}

	virtual void receive(T packet)
	{
		DPRINTF("receive(Packet*)");
		queue.push(packet);
	}
	
	virtual void performStart()
	{
		queue.restart();
		thread.run(this);
	}
	
	virtual void performShutdown()
	{
		queue.notifyShutdown();
		thread.join();
	}

	inline int getCount() 
	{
		return queue.getCount();
	}
	
	/**
	 * -> see Timer::addTimeout for nonspecific description
	 * ATTENTION: this function assumes, that it is usually called by this class's own
	 * thread, as the thread in processLoop always calculates the time of next interruption
	 * before it suspends and this function is not able to interrupt it
	 */
	virtual void addTimeout(Notifiable* n, struct timespec& ts, uint32_t flag = 0)
	{
		mutex.lock();
		TimeoutEntry* e = new TimeoutEntry();
		e->n = n;
		e->timeout = ts;
		e->flag = flag;
		timeouts.push_back(e);
		mutex.unlock();
	}
	
private:
	ConcurrentQueue<T> queue;  /**< contains all elements which were received from previous modules */
	Thread thread;
	list<TimeoutEntry*> timeouts;
	Mutex mutex;	/**< controls access to class variable timeouts */
	
	/**
	 * processes all timeouts in queue which have already timed out
	 * @param time when next timeout will occur
	 * @returns true if timeout was available, false if not
	 */
	bool processTimeouts(struct timespec& nexttimeout)
	{
		struct timespec now;
		addToCurTime(&now, 0);
		bool nexttoset = false;
		
		mutex.lock();
		list<TimeoutEntry*>::iterator iter = timeouts.begin();
		while (iter != timeouts.end()) {
			if (compareTime((*iter)->timeout, now) <= 0) {
				// this entry has already timed out, call it now!
				TimeoutEntry* te = *iter;
				te->n->onTimeout(te->flag);
				iter = timeouts.erase(iter);
				delete te;
				
			} else {
				if (!nexttoset || compareTime(nexttimeout, (*iter)->timeout) > 0) {
					nexttoset = true;
					nexttimeout = (*iter)->timeout;
				}
				iter++;
			}
		}
		mutex.unlock();
		
		return nexttoset;
	}
	
	/**
	 * processes all incoming elements and forwards them to following modules
	 * all timeouts of the following module are controlled here
	 */
	void processLoop()
	{
		T element;
		while (!Module::getExitFlag()) {
			struct timespec nexttimeout;
			if (!processTimeouts(nexttimeout)) {
				if (!queue.pop(&element)) {
					DPRINTF("queue.pop failed - timeout?");
					continue;
				}				
			} else {				
				if (!queue.popAbs(nexttimeout, &element)) {
					DPRINTF("queue.pop failed - timeout?");
					continue;
				}
			}
			
			if (!Source<T>::send(element)) break;
		}	
	}
	
	/**
	 * small wrapper for thread
	 */
	static void* threadWrapper(void *arg)
	{
		ConnectionQueue* self = (ConnectionQueue*)arg;
		DPRINTF("starting thread");

		self->processLoop();
		
		DPRINTF("terminating thread");
		return NULL;
	}
};

#endif

