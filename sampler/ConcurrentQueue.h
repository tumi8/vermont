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

template<class T>
        class ConcurrentQueue
{
public:
        ConcurrentQueue() : queue(), count(0), lock(), semaphore()
        {
        };

        ~ConcurrentQueue()
        {
                if(count != 0) {
                        msg(MSG_ERROR, "ConcurrentQueue: WARNING: freeing non-empty queue - got count: %d", count);
                }
        };

        inline void push(T *t)
        {
                lock.lock();
                queue.push(t);
                lock.unlock();

                semaphore.post();
        };

        inline T *pop()
        {
                T *result;

                semaphore.wait();

                lock.lock();
                result = queue.front();
                queue.pop();
                lock.unlock();

                return result;
        };

        inline int getCount() const
        {
                return count;
        };

protected:
        std::queue<T *> queue;
        int count;
        Lock lock;
        TimeoutSemaphore semaphore;
};

#endif
