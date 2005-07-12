/*
 * PSAMP Reference Implementation
 *
 * TimeoutSemaphore.h
 *
 * Class which encapsulates a semaphore with timeout constraint
 *
 * Author: Michael Drueing <michael@drueing.de>
 *
 */

#ifndef TIMEOUT_SEMAPHORE_H
#define TIMEOUT_SEMAPHORE_H

#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>

class TimeoutSemaphore
{
private:
	pthread_mutex_t mutex;
	pthread_cond_t cond; // waiting threads block on this

	unsigned long count; // the semaphore's value

public:
	// constructs a new Semaphore
	inline TimeoutSemaphore(int initialValue = 0) : count(initialValue)
	{
		pthread_mutex_init(&mutex, 0);
		pthread_cond_init(&cond, 0);
	};

	// deletes a Semaphore.
	inline ~TimeoutSemaphore()
	{
		pthread_cond_destroy(&cond);
		pthread_mutex_destroy(&mutex);
	};

	// Acquire the lock if possible, or wait max. timeout_ms milliseconds
	// for the lock to become available.
	// if the timeout is reaced, return false
	// if the semaphore was successfully acquired, return true
	// a timeout of -1 means infinite wait
	// ***********************
	// ** DEPRECATED, use waitabs() instead
	// ***********************
	inline bool wait(long timeout_ms = -1)
	{
		bool result = true; // this is true unless a timeout happens
		struct timeval tv;
		struct timespec ts;
		int retval;

		pthread_mutex_lock(&mutex);

		// check if the semaphore is unavailable
		if(count == 0) {
			if(timeout_ms == -1) {
				// wait infinite and then acquire the semaphore
				pthread_cond_wait(&cond, &mutex);
				count--;
			} else {
				// calculate absolute time from timeout
				gettimeofday(&tv, 0);
				// add timeout value to the current time
				tv.tv_usec += timeout_ms * 1000L;
				if (tv.tv_usec > 1000000L)
				{
					tv.tv_sec += (tv.tv_usec/1000000L);
					tv.tv_usec %= 1000000L;
				}
				ts.tv_sec = tv.tv_sec;
				ts.tv_nsec = tv.tv_usec * 1000L;

				// block and wait for signal or timeout
				retval = pthread_cond_timedwait(&cond, &mutex, &ts);
				if(retval == ETIMEDOUT) {
					// timeout occured
					result = false;
				} else {
					// we got a signal in time; the semaphore is free to take!
					count--;
				}
			}
		} else {
			// the semaphore is free, so we just decrease the count and return
			count--;
		}

		pthread_mutex_unlock(&mutex);
		return result;
	}

	// like wait() but with absolute time instead of delta. makes things easier!
	// Use this instead of the above function
	inline bool waitAbs(const struct timeval &timeout)
	{
		bool result = true;
		struct timespec ts;

		pthread_mutex_lock(&mutex);

		// check if the semaphore is unavailable
		if(count == 0) {
			// we need the timeout as struct timespec, so convert it here
			TIMEVAL_TO_TIMESPEC(&timeout, &ts);
			
			// block and wait for signal or timeout
			if(pthread_cond_timedwait(&cond, &mutex, &ts) == ETIMEDOUT) {
				// timeout occured
				result = false;
			} else {
				// we got a signal in time; the semaphore is free to acquire!
				count--;
			}
		} else {
			// semaphore is free, so just take it
			count--;
		}
		
		pthread_mutex_unlock(&mutex);
		return result;
	}

	inline void post()
	{
		pthread_mutex_lock(&mutex);

		// increment the semaphore's value
		count++;

		// wake up the next thread waiting for the semaphore
		pthread_cond_signal(&cond);
		
		pthread_mutex_unlock(&mutex);
	};

};

#endif
