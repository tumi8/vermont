/*
 * PSAMP Reference Implementation
 *
 * TimeoutLock.h
 *
 * Class which encapsulates a lock with a timeout.
 * Currently implemented using a Condition Variable.
 *
 * Author: Michael Drueing <michael@drueing.de>
 *
 */
 
#ifndef TIMEOUT_LOCK_H
#define TIMEOUT_LOCK_H

#include "Globals.h"

#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>

class TimeoutLock
{
private:
  pthread_cond_t cond;
  pthread_mutex_t mutex;
  bool locked;
  
public:
  // construct a new lock
  inline TimeoutLock() : locked(false)
  {
    pthread_mutex_init(&mutex, 0);
    pthread_cond_init(&cond, 0);
  }
  
  // destroy the lock
  // TODO: actually check if the lock is held. If so, releasing
  //       it causes unpredictable results!
  inline ~TimeoutLock()
  {
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);
  }
  
  // Acquire the lock or wait until timeout has expired.
  // if the lock was acquired, return true
  // if the timeout was reached without the lock being acquired,
  // return false
  // a timeout of -1 means infinite (i.e. results are equal to a "normal" Lock
  inline bool lock(long timeout_ms = -1)
  {
    bool result;
    struct timeval tv;
    struct timespec ts;
    
    pthread_mutex_lock(&mutex);
    
    if (locked)
    {
      // there's already someone holding the lock
      if (timeout_ms == -1)
      {
        // wait indefinitely
        pthread_cond_wait(&cond, &mutex);
        locked = true;
      }
      else
      {
        // calculate absolute time for timeout
        gettimeofday(&tv, 0);
        // add the timeout value to the current time
        ts.tv_sec = tv.tv_sec;
        ts.tv_nsec = tv.tv_usec * 1000L + timeout_ms * 1000000L;
        if (ts.tv_nsec > 999999999L) // overflow in nsec?
        {
          ts.tv_sec += (ts.tv_nsec / 1000000000L);
          ts.tv_nsec %= 1000000000L;
        }
        // lock and wait for signal or timeout
        if (pthread_cond_timedwait(&cond, &mutex, &ts) == ETIMEDOUT)
        {
          // timeout occured
          locked = false;
        }
        else
        {
          // lock acquired
          locked = true;
        }
      }
    }
    else
    {
      // the lock is not yet acquired
      locked = true;
    }
    
    pthread_mutex_unlock(&mutex);
    return locked;
  }
  
  // unlock the lock
  inline void unlock()
  {
    pthread_mutex_lock(&mutex);
    
    locked = false;             // set to "unlocked"
    pthread_cond_signal(&cond); // wake up next thread waiting to acquire the lock
    
    pthread_mutex_unlock(&mutex);
  }
  
  inline bool try_lock()
  {
    // not yet implemented...
  }
};
 
#endif
