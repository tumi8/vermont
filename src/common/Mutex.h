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

#ifndef MUTEX_H
#define MUTEX_H

#include <pthread.h>
#include <errno.h>

#include "msg.h"

class Mutex
{
	private:
		// the system mutex used (currently using pthread mutexes)
		pthread_mutex_t mutex;

	public:
		// constructs a new Mutex
		inline Mutex() 
		{
			pthread_mutex_t fastmutex = PTHREAD_MUTEX_INITIALIZER;
			mutex = fastmutex;
			pthread_mutex_init(&mutex, 0);
		}

		/**
		 * deletes a Lock.
		 * TODO: use try_lock to see if lock is currently held.
		 * If so, then results are unpredictable!
		 */
		virtual ~Mutex()
		{
			pthread_mutex_destroy(&mutex);
		};

		inline void lock()
		{
			int retval = pthread_mutex_lock(&mutex);
			switch (retval) {
				case 0:
					break;
				case EINVAL:
					THROWEXCEPTION("mutex has not been initialized correctly");
					break;
				case EDEADLK:
					THROWEXCEPTION("deadlock has occured");
					break;
				default:
					THROWEXCEPTION("pthread_mutex_lock returned with unknown failure %d", retval);
					break;
			}
		};

		inline void unlock()
		{
			int retval = pthread_mutex_unlock(&mutex);
			switch (retval) {
				case 0:
					break;
				case EINVAL:
					THROWEXCEPTION("mutex has not been initialized correctly");
					break;
				case EPERM:
					THROWEXCEPTION("calling thread does not own mutex");
					break;
				default:
					THROWEXCEPTION("pthread_mutex_unlock returned with unknown failure %d", retval);
					break;
			}
		};

		inline bool try_lock()
		{
			int retval = pthread_mutex_trylock(&mutex);
			switch (retval) {
				case 0:
					return true;
				case EBUSY:
					return false;
				case EINVAL:
					THROWEXCEPTION("mutex has not been initialized correctly");
					break;
				default:
					THROWEXCEPTION("pthread_mutex_trylock returned with unknown failure %d", retval);
					break;
			}
			return false;
		};
};

#endif
