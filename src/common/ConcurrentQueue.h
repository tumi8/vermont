/*
 * Thread-safe (concurrent) queue implementation
 * The implementation of the BaseQueue makes it thread-safe
 *
 * Make sure that size of T is 4 bytes on 32 bit operating systems
 * and 8 bytes on 64 bit operating systems accordingly when using
 * QueueType LOCKFREE_MULTI
 *
 * Author: Michael Drueing <michael@drueing.de>
 *
 */

#ifndef CONCURRENT_QUEUE_H
#define CONCURRENT_QUEUE_H

#include <string>
#include "BaseQueue.h"
#include "STLQueue.h"
#include "LockfreeSingleQueue.h"
#include "LockfreeMultiQueue.h"
#include "TimeoutSemaphore.h"
#include "msg.h"

template<class T>
class ConcurrentQueue
{
	public:
		/**
		 * default queue size
		 */
		static const int DEFAULT_QUEUE_SIZE = 1000;

		/**
		 * types of queues
		 */
		enum QUEUETYPES {STL, LOCKFREE_SINGLE, LOCKFREE_MULTI};

		ConcurrentQueue(int qType = STL, int maxEntries = DEFAULT_QUEUE_SIZE):
			pushedCount(0), poppedCount(0), popSemaphore(), pushSemaphore(maxEntries)
		{
			this->maxEntries = maxEntries;

			switch(qType){
				case STL:
					queue = new STLQueue<T>();
					break;
				case LOCKFREE_SINGLE:
					queue = new LockfreeSingleQueue<T>(maxEntries);
					break;
				case LOCKFREE_MULTI:
					queue = new LockfreeMultiQueue<T>(maxEntries);
					break;
				default:
					THROWEXCEPTION("Unknown Queue Type");
			}
		};

		~ConcurrentQueue()
		{
			if(getCount() != 0) {
				msg(MSG_DEBUG, "WARNING: freeing non-empty queue - got count: %d", getCount());
			}
		};

		void setOwner(std::string name)
		{
			ownerName = name;
		}

		inline void push(T t)
		{
			DPRINTFL(MSG_VDEBUG, "(%s) trying to push element (%d elements in queue)", ownerName.c_str(), getCount());
#if defined(DEBUG)
			bool waiting = false;
			if (pushSemaphore.getCount() == 0) {
				waiting = true;
				DPRINTFL(MSG_DEBUG, "(%s) queue is full with %d elements, waiting ...", ownerName.c_str(), getCount());
			}
#endif
			if (!pushSemaphore.wait()) {
				DPRINTF("(%s) failed to push element, program is being shut down?", ownerName.c_str());
				return;
			}
#if defined(DEBUG)
			if (waiting) DPRINTF("(%s) pushing element now", ownerName.c_str());
#endif

			if(!queue->push(t))
				THROWEXCEPTION("Could not push element");
			pushedCount++;

			popSemaphore.post();
			DPRINTFL(MSG_VDEBUG, "(%s) element pushed (%d elements in queue)", ownerName.c_str(), maxEntries-pushSemaphore.getCount(), pushSemaphore.getCount(), maxEntries);
		};

		inline bool pop(T* res)
		{
			DPRINTFL(MSG_VDEBUG, "(%s) trying to pop element (%d elements in queue)",
					(ownerName.empty() ? "<owner not set>" : ownerName.c_str()),
					maxEntries-pushSemaphore.getCount());

			if (!popSemaphore.wait()) {
				DPRINTF("(%s) failed to pop element, program is being shut down?", ownerName.c_str());
				return false;
			}

			if(!queue->pop(res))
				THROWEXCEPTION("Could not pop element");
			poppedCount++;

			pushSemaphore.post();

			DPRINTFL(MSG_VDEBUG, "(%s) element popped", ownerName.c_str());

			return true;
		};

		// try to pop an entry from the queue before timeout occurs
		// if successful, res will hold the popped entry and true will be returned
		// of the timeout has been reached, res will be set to NULL and false will be returned
		inline bool pop(long timeout_ms, T *res)
		{
			DPRINTFL(MSG_VDEBUG, "(%s) trying to pop element (%d elements in queue)", ownerName.c_str(), getCount());
			// try to get an item from the queue
			if(!popSemaphore.wait(timeout_ms)) {
				// timeout occured
				DPRINTFL(MSG_VDEBUG, "(%s) timeout", ownerName.c_str());
				return false;
			}

			// popSemaphore.wait() succeeded, now pop the frontmost element
			if(!queue->pop(res))
				THROWEXCEPTION("Could not pop element");
			poppedCount++;

			pushSemaphore.post();

			DPRINTFL(MSG_VDEBUG, "(%s) element popped", ownerName.c_str());

			return true;
		}

		// like pop above, but with absolute time instead of delta.
		// use this instead of the above, makes things easier!
		inline bool popAbs(const struct timeval &timeout, T *res)
		{
			DPRINTFL(MSG_VDEBUG, "(%s) trying to pop element (%d elements in queue)", ownerName.c_str(), getCount());
			
			if (!popSemaphore.waitAbs(timeout)) {
				// timeout occured
				DPRINTFL(MSG_VDEBUG, "(%s) timeout or program shutdown", ownerName.c_str());
				*res = 0;
				return false;
			}

			// popSemaphore.waitAbs() succeeded, now pop the frontmost element
			if(!queue->pop(res))
				THROWEXCEPTION("Could not pop element");
			poppedCount++;

			pushSemaphore.post();

			DPRINTFL(MSG_VDEBUG, "(%s) element popped", ownerName.c_str());

			return true;
		}
		
		// like pop above, but with absolute time instead of delta.
		// use this instead of the above, makes things easier!
		inline bool popAbs(const struct timespec& timeout, T *res)
		{
			DPRINTFL(MSG_VDEBUG, "(%s) trying to pop element (%d elements in queue)", ownerName.c_str(), getCount());
		
			if (!popSemaphore.waitAbs(timeout)) {
				// timeout occured
				DPRINTFL(MSG_VDEBUG, "(%s) timeout or program shutdown", ownerName.c_str());
				*res = 0;
		
				return false;
			}

			// popSemaphore.waitAbs() succeeded, now pop the frontmost element
			if(!queue->pop(res))
				THROWEXCEPTION("Could not pop element");
			poppedCount++;

			pushSemaphore.post();

			DPRINTFL(MSG_VDEBUG, "(%s) element popped", ownerName.c_str());

			return true;
		}

		inline int getCount() const
		{
			return pushedCount - poppedCount;
		};
		
		
		/**
		 * after calling this function, queue will not block again but return
		 * all functions with an error
		 * (useful for shutdown of this instance)
		 */
		void notifyShutdown() 
		{
			popSemaphore.notifyShutdown();
			pushSemaphore.notifyShutdown();
		}
		
		
		/**
		 * activates all thread-locking functionality inside the queue again
		 */
		void restart()
		{
			popSemaphore.restart();
			pushSemaphore.restart();
			queue->reset();
		}

		int pushedCount;
		int poppedCount;
		int maxEntries;

	protected:
		BaseQueue<T>* queue;
		TimeoutSemaphore popSemaphore;
		TimeoutSemaphore pushSemaphore;
		std::string ownerName;
};

#endif
