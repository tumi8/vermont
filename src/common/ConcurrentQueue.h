/*
 * Thread-safe (concurrent) queue implementation
 * The implementation of the BaseQueue makes it thread-safe
 *
 * Make sure that size of T is 4 bytes on 32 bit operating systems
 * and 8 bytes on 64 bit operating systems accordingly when using
 * QueueTypes LOCKFREE_SINGLE or LOCKFREE_MULTI
 *
 * Author: Michael Drueing <michael@drueing.de>
 *
 */

#ifndef CONCURRENT_QUEUE_H
#define CONCURRENT_QUEUE_H

#include <string>
#include "BaseQueue.h"
#include "STLQueue.h"
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
			pushedCount(0), poppedCount(0), count(0), popSemaphore(), pushSemaphore(maxEntries)
		{
			this->maxEntries = maxEntries;

			switch(qType){
				case STL:
					queue = new STLQueue<T>();
					break;
				case LOCKFREE_SINGLE:
					THROWEXCEPTION("LOCKFREE_SINGLE not yet implemented");
					break;
				case LOCKFREE_MULTI:
					THROWEXCEPTION("LOCKFREE_MULTI not yet implemented");
					break;
				default:
					THROWEXCEPTION("Unknown Queue Type");
			}
		};

		~ConcurrentQueue()
		{
			if(count != 0) {
				msg(MSG_DEBUG, "WARNING: freeing non-empty queue - got count: %d", count);
			}
		};

		void setOwner(std::string name)
		{
			ownerName = name;
		}

		inline void push(T t)
		{
			DPRINTFL(MSG_VDEBUG, "(%s) trying to push element (%d elements in queue)", ownerName.c_str(), count);
#if defined(DEBUG)
			bool waiting = false;
			if (pushSemaphore.getCount() == 0) {
				waiting = true;
				DPRINTFL(MSG_DEBUG, "(%s) queue is full with %d elements, waiting ...", ownerName.c_str(), count);
			}
#endif
			if (!pushSemaphore.wait()) {
				DPRINTF("(%s) failed to push element, program is being shut down?", ownerName.c_str());
				return;
			}
#if defined(DEBUG)
			if (waiting) DPRINTF("(%s) pushing element now", ownerName.c_str());
#endif

			queue->push(t);
			pushedCount++;
			count++;

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

			queue->pop(res);
			poppedCount++;
			count--;

			pushSemaphore.post();

			DPRINTFL(MSG_VDEBUG, "(%s) element popped", ownerName.c_str());

			return true;
		};

		// try to pop an entry from the queue before timeout occurs
		// if successful, res will hold the popped entry and true will be returned
		// of the timeout has been reached, res will be set to NULL and false will be returned
		inline bool pop(long timeout_ms, T *res)
		{
			DPRINTFL(MSG_VDEBUG, "(%s) trying to pop element (%d elements in queue)", ownerName.c_str(), count);
			// try to get an item from the queue
			if(!popSemaphore.wait(timeout_ms)) {
				// timeout occured
				DPRINTFL(MSG_VDEBUG, "(%s) timeout", ownerName.c_str());
				return false;
			}

			// popSemaphore.wait() succeeded, now pop the frontmost element
			queue->pop(res);
			poppedCount++;
			count--;

			pushSemaphore.post();

			DPRINTFL(MSG_VDEBUG, "(%s) element popped", ownerName.c_str());

			return true;
		}

		// like pop above, but with absolute time instead of delta.
		// use this instead of the above, makes things easier!
		inline bool popAbs(const struct timeval &timeout, T *res)
		{
			DPRINTFL(MSG_VDEBUG, "(%s) trying to pop element (%d elements in queue)", ownerName.c_str(), count);
			
			if (!popSemaphore.waitAbs(timeout)) {
				// timeout occured
				DPRINTFL(MSG_VDEBUG, "(%s) timeout or program shutdown", ownerName.c_str());
				*res = 0;
				return false;
			}

			// popSemaphore.waitAbs() succeeded, now pop the frontmost element
			queue->pop(res);
			poppedCount++;
			count--;

			pushSemaphore.post();

			DPRINTFL(MSG_VDEBUG, "(%s) element popped", ownerName.c_str());

			return true;
		}
		
		// like pop above, but with absolute time instead of delta.
		// use this instead of the above, makes things easier!
		inline bool popAbs(const struct timespec& timeout, T *res)
		{
			DPRINTFL(MSG_VDEBUG, "(%s) trying to pop element (%d elements in queue)", ownerName.c_str(), count);
		
			if (!popSemaphore.waitAbs(timeout)) {
				// timeout occured
				DPRINTFL(MSG_VDEBUG, "(%s) timeout or program shutdown", ownerName.c_str());
				*res = 0;
		
				return false;
			}

			// popSemaphore.waitAbs() succeeded, now pop the frontmost element
			queue->pop(res);
			poppedCount++;
			count--;

			pushSemaphore.post();

			DPRINTFL(MSG_VDEBUG, "(%s) element popped", ownerName.c_str());

			return true;
		}

		inline int getCount() const
		{
			return count;
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
		}

		int pushedCount;
		int poppedCount;
		int maxEntries;

	protected:
		BaseQueue<T>* queue;
		volatile int count;
		TimeoutSemaphore popSemaphore;
		TimeoutSemaphore pushSemaphore;
		std::string ownerName;
};

#endif
