#include "CountingSemaphore.h"
#include "msg.h"

#include <pthread.h>

CountingSemaphore::CountingSemaphore (unsigned int startvalue)
{
	if (pthread_mutex_init (&mutex, NULL) != 0) {
		perror ("initialization of mutex failed");
		return;
	}
	if (pthread_cond_init (&cond, NULL) != 0) {
		perror ("initialization of condition variable failed");
		return;
	}
	val = startvalue;
}

CountingSemaphore::~CountingSemaphore () {
	if (pthread_mutex_destroy (&mutex) != 0) {
		perror ("destroy of mutex failed");
		return;
	}
	if (pthread_cond_destroy (&cond) != 0) {
		perror ("destroy of condition variable failed");
		return;
	}
}

void CountingSemaphore::dec (unsigned int dec) 
{
	if (pthread_mutex_lock (&mutex) != 0)
		THROWEXCEPTION("lock of mutex failed");

	while (val < dec) {
		if (pthread_cond_wait (&cond, &mutex) != 0)
			THROWEXCEPTION("condition wait failed");
	}
	val -= dec;

	if (pthread_mutex_unlock (&mutex) != 0)
		THROWEXCEPTION("unlock of mutex failed");
}

void CountingSemaphore::inc (unsigned int inc) {
	if (pthread_mutex_lock (&mutex) != 0) {
		THROWEXCEPTION("lock of mutex failed");
	}

	val += inc;
	if (pthread_cond_broadcast (&cond) != 0) {
		perror ("condition broadcast failed");
		return;
	}

	if (pthread_mutex_unlock (&mutex) != 0) {
		perror ("unlock of mutex failed");
		return;
	}
}
