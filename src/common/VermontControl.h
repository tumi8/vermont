#ifndef VERMONTCONTROL_H_
#define VERMONTCONTROL_H_

#include <semaphore.h>
#include "common/TimeoutSemaphore.h"

extern bool run_program;
extern bool reload_config;

extern sem_t mainSemaphore;
extern TimeoutSemaphore timeoutsem;

void initiateShutdown();
void wakeupMainThread();

#endif /*VERMONTCONTROL_H_*/
