#ifndef VERMONTCONTROL_H_
#define VERMONTCONTROL_H_

#ifdef __APPLE__
#include <osdep/osx/sem_timedwait.h>
#else
#include <semaphore.h>
#endif

#include "common/TimeoutSemaphore.h"

extern bool run_program;
extern bool reload_config;

#ifdef __APPLE__
extern semaphore_t mainSemaphore;
#else 
extern sem_t mainSemaphore;
#endif 
extern TimeoutSemaphore timeoutsem;

void initiateShutdown();
void wakeupMainThread();

#endif /*VERMONTCONTROL_H_*/
