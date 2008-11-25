/*
 * PSAMP Reference Implementation
 *
 * ConcurrentQueue.h
 *
 * Thread-safe (concurrent) queue implementation
 *
 * Author: Michael Drueing <michael@drueing.de>
 *
 */

#ifndef CONCURRENT_QUEUE_H
#define CONCURRENT_QUEUE_H

#include <queue>
#include <string>
#include "Mutex.h"
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

		ConcurrentQueue(int maxEntries = DEFAULT_QUEUE_SIZE) 
			: pushedCount(0), poppedCount(0), queue(), count(0), lock(), popSemaphore(), pushSemaphore(maxEntries)
		{
			this->maxEntries = maxEntries;
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

			lock.lock();
			queue.push(t);
			pushedCount++;
			count++;
			lock.unlock();

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

			lock.lock();
			*res = queue.front();
			queue.pop();
			poppedCount++;
			count--;
			lock.unlock();

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
			lock.lock();
			*res = queue.front();
			queue.pop();
			poppedCount++;
			count--;
			lock.unlock();

			pushSemaphore.post();

			DPRINTFL(MSG_VDEBUG, "(%s) element popped", ownerName.c_str());

			return true;
		}

		// like pop above, but with absolute time instead of delta.
		// use this instead of the above, makes things easier!
		inline bool popAbs(const struct timeval &timeout, T *res)
		{
			DPRINTFL(MSG_VDEBUG, "(%s) trying to pop element (%d elements in queue)", ownerName.c_str(), count);
			
			if (popSemaphore.waitAbs(timeout)) {
				// popSemaphore.wait() succeeded, now pop the frontmost element
				lock.lock();
				*res = queue.front();
				queue.pop();
				poppedCount++;
				count--;
				lock.unlock();

				pushSemaphore.post();

				DPRINTFL(MSG_VDEBUG, "(%s) element popped", ownerName.c_str());

				return true;
			} else {
				// timeout occured
				DPRINTFL(MSG_VDEBUG, "(%s) timeout or program shutdown", ownerName.c_str());
				*res = 0;

				return false;
			}
		}
		
		// like pop above, but with absolute time instead of delta.
		// use this instead of the above, makes things easier!
		inline bool popAbs(const struct timespec& timeout, T *res)
		{
			DPRINTFL(MSG_VDEBUG, "(%s) trying to pop element (%d elements in queue)", ownerName.c_str(), count);
		
			if (popSemaphore.waitAbs(timeout)) {
				// popSemaphore.wait() succeeded, now pop the frontmost element
				lock.lock();
				*res = queue.front();
				queue.pop();
				poppedCount++;
				count--;
				lock.unlock();
		
				pushSemaphore.post();
		
				DPRINTFL(MSG_VDEBUG, "(%s) element popped", ownerName.c_str());
		
				return true;
			}
			else {
				// timeout occured
				DPRINTFL(MSG_VDEBUG, "(%s) timeout or program shutdown", ownerName.c_str());
				*res = 0;
		
				return false;
			}
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
		std::queue<T> queue;
		volatile int count;
		Mutex lock;
		TimeoutSemaphore popSemaphore;
		TimeoutSemaphore pushSemaphore;
		std::string ownerName;
};

#endif
