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
#include "Globals.h"
#include "Lock.h"
#include "TimeoutLock.h"
#include "Semaphore.h"

template<class T>
class ConcurrentQueue
{
public:
  ConcurrentQueue() : queue(), count(0), lock(), semaphore()
  {
  };

  ~ConcurrentQueue()
  {
    if (count != 0)
      LOG("Queue: WARNING: freeing non-empty queue!\n");
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

  inline bool try_pop(T **t)
  {
    T *result;

    if (!semaphore.try_wait())
      return false;
    
    lock.lock();
    result = queue.front();
    queue.pop();
    lock.unlock();

    *t = result;

    return true;
  };

  inline int getCount() const
  {
    return count;
  };

protected:
  std::queue<T *> queue;
  int count;
  TimeoutLock lock;
  Semaphore semaphore;
};

#endif
