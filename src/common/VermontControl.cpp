#include "VermontControl.h"

#include "msg.h"

bool run_program = true;
bool reload_config = false;

#ifdef __APPLE__
semaphore_t mainSemaphore;
#else 
sem_t mainSemaphore;
#endif 
TimeoutSemaphore timeoutsem;


/**
 * initiates shutdown of Vermont
 */
void initiateShutdown()
{
	run_program = false;
	wakeupMainThread();
}


void wakeupMainThread()
{
#ifdef __APPLE__
	if (semaphore_signal(mainSemaphore) != KERN_SUCCESS) THROWEXCEPTION("failed to execute sem_post");
#else
	if (sem_post(&mainSemaphore) == -1) THROWEXCEPTION("failed to execute sem_post");
#endif
	timeoutsem.post();
}
