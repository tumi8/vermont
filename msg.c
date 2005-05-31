/*
 this is vermont.
 released under GPL v2

 (C) by Ronny T. Lampert

 */
#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>
#include "msg.h"

#ifdef __cplusplus
extern "C" {
#endif

static int msg_level=MSG_DEFAULT;
static char *MSG_TAB[]={ "FATAL  ", "VERMONT", "ERROR  ", "DEBUG  ", "INFO   ", 0};

/* we need to serialize for msg_stat() */
static pthread_mutex_t stat_lock=PTHREAD_MUTEX_INITIALIZER;
static FILE *stat_file;

/*
 the main logging routine

 we don't have to lock, because glibc's printf() system is doing it already

 if you, however, change this function to a custom backend, you WILL have to lock
 because msg() can and will be called by concurrent threads!
 */
void msg(int level, char *fmt, ...)
{
	/* nummerically higher value means lower priority */
	if (level > msg_level) {
		return;
	} else {
		va_list args;
		printf("%s: ", MSG_TAB[level]);
		va_start(args, fmt);
		vprintf(fmt, args);
		va_end(args);
		printf("\n");
	}
}


void msg_setlevel(int level)
{
	msg_level=level;
}


/*
 output statistics; usually to file

 call msg_stat_setup() before
 we need a lock (can be called concurrently) to not clutter it all up
 keep critical section as small as possible
 */
int msg_stat(char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);

	pthread_mutex_lock(&stat_lock);
	vfprintf(stat_file, fmt, args);
	pthread_mutex_unlock(&stat_lock);

	va_end(args);

	return 0;
}


/* this is future compatible to interact with the system in an ioctl() style */
int msg_stat_setup(int mode, FILE *f)
{
	if(f) {
		pthread_mutex_lock(&stat_lock);
		stat_file=f;
		pthread_mutex_unlock(&stat_lock);

		return 0;
	}

	return 1;

}

#ifdef __cplusplus
}
#endif
