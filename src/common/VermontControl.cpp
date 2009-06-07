#include "VermontControl.h"

#include "msg.h"

bool run_program = true;
bool reload_config = false;

sem_t mainSemaphore;


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
	if (sem_post(&mainSemaphore) == -1) THROWEXCEPTION("failed to execute sem_post");
}
