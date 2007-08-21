#ifndef __SEMAPHOR_HPP__
#define __SEMAPHOR_HPP__

#include <pthread.h>

class CountingSemaphore {
private:
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	unsigned int val;
public:
	CountingSemaphore (unsigned int startvalue);
	~CountingSemaphore();
	
	void dec (unsigned int dec);
	void inc (unsigned int inc);
};

#endif
