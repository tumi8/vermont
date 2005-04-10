/*
 * PSAMP Reference Implementation
 *
 * Lock.h
 *
 * Class which encapsulates a simple lock
 *
 * Author: Michael Drueing <michael@drueing.de>
 *
 */

#ifndef LOCK_H
#define LOCK_H

#include <pthread.h>

class Lock
{
private:
  // the system mutex used (currently using pthread mutexes)
  pthread_mutex_t mutex;
  
public:
  // constructs a new Lock
  inline Lock()
  {
    pthread_mutex_init(&mutex, NULL);
  };
  
  // deletes a Lock.
  // TODO: use try_lock to see if lock is currently held. If so,
  // then results are unpredictable!
  inline ~Lock()
  {
    pthread_mutex_destroy(&mutex);
  };
  
  inline void lock()
  {
    pthread_mutex_lock(&mutex);
  };

  inline void unlock()
  {
    pthread_mutex_unlock(&mutex);
  };

  inline bool try_lock()
  {
    return (pthread_mutex_trylock(&mutex) == 0);
  };
};

#endif
