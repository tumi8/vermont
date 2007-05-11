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
#include "Lock.h"
#include "TimeoutSemaphore.h"
#include "msg.h"

template<class T>
        class ConcurrentQueue
{
public:
        ConcurrentQueue() : pushedCount(0), poppedCount(0), queue(), count(0), lock(), semaphore()
        {
        };

        ~ConcurrentQueue()
        {
                if(count != 0) {
                        msg(MSG_DEBUG, "ConcurrentQueue: WARNING: freeing non-empty queue - got count: %d", count);
                }
        };

        inline void push(T t)
        {
                lock.lock();
                queue.push(t);
		pushedCount++;
		count++;
                lock.unlock();

                semaphore.post();
        };

        inline T pop()
        {
                T result;

                semaphore.wait();

                lock.lock();
                result = queue.front();
                queue.pop();
		poppedCount++;
		count--;
                lock.unlock();

                return result;
        };

	// try to pop an entry from the queue before timeout occurs
	// if successful, res will hold the popped entry and true will be returned
	// of the timeout has been reached, res will be set to NULL and false will be returned
	// *******************
	// *** DEPRECATED, use pop_abs instead
	// *******************
	inline bool pop(long timeout_ms, T *res)
	{
		// try to get an item from the queue
		if(!semaphore.wait(timeout_ms)) {
			// timeout occured
			return false;
		}
		
		// semaphore.wait() succeeded, now pop the frontmost element
		lock.lock();
		*res = queue.front();
		queue.pop();
		poppedCount++;
		count--;
		lock.unlock();

		return true;
	}

	// like pop above, but with absolute time instead of delta.
	// use this instead of the above, makes things easier!
	inline bool popAbs(const struct timeval &timeout, T *res)
	{
		if(!semaphore.waitAbs(timeout)) {
			// timeout occured
			*res = 0;
			return false;
		}

		// semaphore.wait() succeeded, now pop the frontmost element
		lock.lock();
		*res = queue.front();
		queue.pop();
		poppedCount++;
		count--;
		lock.unlock();

		return true;
	}

        inline int getCount() const
        {
                return count;
        };

	int pushedCount;
	int poppedCount;

protected:
        std::queue<T> queue;
        int count;
        Lock lock;
        TimeoutSemaphore semaphore;
};

#endif
