/*
 * PSAMP Reference Implementation
 *
 * Thread.h
 *
 * Implementation of a base class for a thread
 *
 * Author: Michael Drueing <michael@drueing.de>
 *
 */

#ifndef THREAD_H
#define THREAD_H

#include <pthread.h>
#include "Globals.h"

typedef void* (*ThreadFunc)(void *);

class Thread
{
public:
  Thread(ThreadFunc threadFunction) : exitFlag(false), f(threadFunction)
  {
  };

  bool run(void *threadData)
  {
    //data = threadData;
    return pthread_create(&thread, NULL, f, threadData) == 0;
  };

  void *join()
  {
    void *result = NULL;
    if (pthread_join(thread, &result))
      LOG("Thread: ERROR: Join failed\n");
      
    return result;
  };

  bool detach()
  {
    return (pthread_detach(thread) == 0);
  }
  
  ~Thread()
  {
  };

  bool exitFlag;
  //void *data;
  
private:
  pthread_t thread;
  ThreadFunc f;
};

#endif
