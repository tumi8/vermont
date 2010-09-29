/*
 * PSAMP Reference Implementation
 *
 * TimeoutSemaphore.h
 *
 * Class which encapsulates a semaphore with timeout constraint
 *
 * original version: Michael Drueing <michael@drueing.de>
 * new version: Tobias Limmer <tobias.limmer@informatik.uni-erlangen.de>
 */

#ifndef TIMEOUT_SEMAPHORE_H
#define TIMEOUT_SEMAPHORE_H

#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <semaphore.h>
#include <stdio.h>
#include <string.h>

#include "msg.h"
#include "Time.h"

class TimeoutSemaphore
{
private:
	static const int STANDARD_TIMEOUT = 100; // when no timeout is given by calling function, this amount of ms will be waited until the exitFlag is checked
	sem_t* sem;

	bool exitFlag;	/**< is set to true when semaphore is to be fshut down **/
	
	struct timespec timeout;  /** used by wait to limit access to time-functions */

	sem_t last_sem;
public:
	TimeoutSemaphore(int initialValue = 0)
		: exitFlag(false)
	{
		sem = new sem_t;

		timeout.tv_sec = 0;
		timeout.tv_nsec = 0;

		int retval = sem_init(sem, 0, initialValue);

		if (retval != 0) {
			THROWEXCEPTION("failed to initialize semaphore, sem_init exited with code %d", errno);
		}

	};

	virtual ~TimeoutSemaphore()
	{
		int retval = sem_destroy(sem);
		if (retval != 0) {
			THROWEXCEPTION("given semaphore is not valid, failed to destroy it");	
		}
		delete sem;
	}
	
	// Acquire the lock if possible, or wait max. timeout_ms milliseconds
	// for the lock to become available.
	// if the timeout is reaced, return false
	// if the semaphore was successfully acquired, return true
	// a timeout of -1 means infinite wait
	inline bool wait(long timeout_ms = -1)
	{
		struct timespec ts;
		int retval;
		
		// if program requested to shut down, just return a failure
		if (exitFlag) return false;

		if (timeout_ms >= 0) {
			// wait until timeout
			addToCurTime(&ts, timeout_ms);
			retval = sem_timedwait(sem, &ts);
			if (retval != 0) {
				switch (errno) {
					case EINVAL:
						THROWEXCEPTION("semaphore or timeout values are invalid");
						return false;
						break;
					default:
						// semaphore could not be aquired because of several reasons, but none are fatal
						DPRINTFL(MSG_VDEBUG, "timedwait (<0) returned with %d", errno);
						return false;
				}
			}
		} else {
		    // wait and check the exitFlag regularly
		    do {
				if (timeout.tv_nsec >= 1000000000) {
					msg(MSG_FATAL, "You have just seen a bug. in TimeoutSemarphore:wait(): timeout.tv_nsec is too big: %lu. Please fix this! I will perform some sanatation, but this does not fix the real error!", timeout.tv_nsec);
					timeout.tv_nsec = 999999999;
				}
				retval = sem_timedwait(sem, &timeout);
				if (retval != 0 && errno != ETIMEDOUT) {
					DPRINTFL(MSG_VDEBUG, "timedwait (>=0) returned with %d: %s", errno, strerror(errno));
					switch (errno) {
						case EINVAL:
						/*
							char text[1000];
							char tmp[10];
							strcpy(text, "last_sem: ");
							for (unsigned int i=0; i<sizeof(sem_t); i++) {
								sprintf(tmp, "%hhX", ((char*)&sem)[i]);
								strcat(text, tmp);
							}
							strcat(text, ", sem: ");
							for (unsigned int i=0; i<sizeof(sem_t); i++) {
								sprintf(tmp, "%hhX", ((char*)&last_sem)[i]);
								strcat(text, tmp);
							}
						*/						 
						THROWEXCEPTION("semaphore or timeout is invalid: %s", strerror(errno));
						return false;
						break;
						default:
						// semaphore could not be aquired because of several reasons, but none are fatal
						DPRINTFL(MSG_VDEBUG, "timedwait (>=0) returned with %d", errno);
					}
				}
				if (errno == ETIMEDOUT) {
					// calculate absolute time from timeout
					struct timespec tmp;
					addToCurTime(&tmp, STANDARD_TIMEOUT);
					// attention: next command may collide between threads, but collision does not matter
					timeout = tmp;
				}
	
				// if program was shutdown, exit without success
				if (exitFlag) {
					DPRINTFL(MSG_VDEBUG, "exitFlag is set", errno);
					return false;
				}
		    } while (retval != 0);
		}
		
		return true;
	}

	
	/**
	 * see documentation for waitAbs(struct timespec)
	 * has same functionality, just different parameter
	 */
	inline bool waitAbs(const struct timeval& timeout)
	{
		struct timespec ts;
		
		TIMEVAL_TO_TIMESPEC(&timeout, &ts);
		
		return waitAbs(ts);
	}

	// like wait() but with absolute time instead of delta. makes things easier!
	// Use this instead of the above function
	inline bool waitAbs(const struct timespec& ts)
	{
		int retval;

		// if program requested to shut down, just return a failure
		if (exitFlag) return false;

		// wait until timeout
		retval = sem_timedwait(sem, &ts);
		switch (retval) {
			case 0:
				return true;
				break;
			case EINVAL:
				THROWEXCEPTION("semaphore is invalid");
				return false;
				break;
			default:
				// semaphore could not be aquired because of several reasons, but none are fatal
				DPRINTFL(MSG_VDEBUG, "timedwait returned with %d", retval);
				return false;
		}

		// this statement should not be reached
		return false;
	}


	/**
	 * increases the semaphore's value by 1
	 */
	inline void post()
	{
	    int retval = sem_post(sem);

	    if (retval != 0) {
		THROWEXCEPTION("semaphore is invalid, sem_post returned with %d", errno);
	    }
	}

	/**
	 * returns the current value of the semaphore
	 */
	inline int getCount()
	{
	    int val;
	    int retval = sem_getvalue(sem, &val);

	    if (retval != 0) {
		THROWEXCEPTION("semaphore is invalid, sem_getvalue returned with %d", errno);
	    }
	
	    return val;
	}


	/**
	 * shuts down the semaphore: all waiting threads will be restarted
	 */
	void notifyShutdown()
	{
	    DPRINTF("shutting down");
	    exitFlag = true;
	}

	/**
	 * when semaphore is shut down, it can be restarted again using this method
	 */
	void restart()
	{
	    DPRINTF("restarting");
	    exitFlag = false;
	}
};

#endif
