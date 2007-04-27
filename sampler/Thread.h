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

#include "msg.h"

#include <pthread.h>

typedef void* (*ThreadFunc)(void *);

class Thread
{
public:
	Thread(ThreadFunc threadFunction) : exitFlag(false), thread_created(false), f(threadFunction)
	{
        };

bool run(void *threadData)
{
	// remember that run() has been called
	thread_created = true;

	//data = threadData;
	return(pthread_create(&thread, NULL, f, threadData) == 0);
};

void *join()
{
	// do not attempt to join if run() had not yet been called
	if (!thread_created) return 0;

	void *result=NULL;
        if(!thread || pthread_join(thread, &result)) {
                msg(MSG_ERROR, "Thread: joining failed");
        }
        return result;
};

bool detach()
{
	// do not attempt to detach if run() had not yet been called
	if (!thread_created) return 0;

	return (pthread_detach(thread) == 0);
}

~Thread()
{
};

bool exitFlag;
//void *data;

private:
	pthread_t thread;
	bool thread_created; /**< true after Thread::run() was called */
	ThreadFunc f;
};

#endif
