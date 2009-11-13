#ifndef __SEMAPHOR_HPP__
#define __SEMAPHOR_HPP__

#include "msg.h"

#include <pthread.h>

class CountingSemaphore {
private:
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	unsigned int val;

	bool exitFlag;

	static const int STANDARD_TIMEOUT = 200;
public:
	CountingSemaphore (unsigned int startvalue);
	~CountingSemaphore();
	
	/** decrement the semaphore
	 *  \param dec value on which the semaphore should be decremented
	 *  \param timeout_ms optional timeout to wait for decrementing the semaphore
	 * 		      (-1 means no timeout)
	 *  \return true if semaphore could be decremented, else false
	 */
	bool dec (unsigned int dec, long timeout_ms = STANDARD_TIMEOUT);

	/** increment the semaphore
	 *  \param inc value on which the semaphore is incremented
	 */
	void inc (unsigned int inc);

	inline void shutdown()
	{
	    DPRINTF("shutting down");
	    exitFlag = true;
	}
};

#endif
