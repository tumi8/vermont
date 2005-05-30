/*
 * PSAMP Reference Implementation
 *
 * Semaphore.h
 *
 * Class which encapsulates a semaphore
 *
 * Author: Michael Drueing <michael@drueing.de>
 *
 */

#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <semaphore.h>

class Semaphore
{
private:
	// the system semaphore used (currently using pthread semaphores)
	sem_t semaphore;

public:
	// constructs a new Semaphore
	inline Semaphore(int initialValue = 0)
	{
		sem_init(&semaphore, 0, initialValue);
	};

	// deletes a Semaphore.
	inline ~Semaphore()
	{
		sem_destroy(&semaphore);
	};

	inline void wait()
	{
		sem_wait(&semaphore);
	};

	inline void post()
	{
		sem_post(&semaphore);
	};

};

#endif
