#ifndef VERMONTCONTROL_H_
#define VERMONTCONTROL_H_


#include <semaphore.h>


extern bool run_program;
extern bool reload_config;

extern sem_t mainSemaphore;

void initiateShutdown();
void wakeupMainThread();

#endif /*VERMONTCONTROL_H_*/
