/*
 this is vermont.
 released under GPL v2

 (C) by Ronny T. Lampert

 */
#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "msg.h"

#ifdef __cplusplus
extern "C" {
#endif

static int msg_level=MSG_DEFAULT;
static char *MSG_TAB[]={ "FATAL  ", "VERMONT", "ERROR  ", "DEBUG  ", "INFO   ", 0};

/* we need to serialize for msg_stat() */
static pthread_mutex_t stat_lock = PTHREAD_MUTEX_INITIALIZER;
static FILE *stat_file;

/* the log functions which the message logger thread calls and stuff needed for them */
static pthread_mutex_t log_array_lock = PTHREAD_MUTEX_INITIALIZER;
static LOGFUNCTION log_functions[MAX_LOG_FUNCTIONS] = { 0 };
static void *log_function_params[MAX_LOG_FUNCTIONS] = { 0 };
static int num_log_functions = 0;
/* each log_timeout, the logger thread will call all registered logging functions */
static struct timespec log_timeout = { tv_sec: 1, tv_nsec: 0 };
static pthread_t log_thread;

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
        /* have to check if subsys is on. Else just ignore */
        if(stat_file) {
                va_list args;
                va_start(args, fmt);

                pthread_mutex_lock(&stat_lock);
                vfprintf(stat_file, fmt, args);
                fputs("\n", stat_file);
                pthread_mutex_unlock(&stat_lock);

                va_end(args);
        }

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

void msg_thread_add_log_function(LOGFUNCTION f, void *param)
{
  pthread_mutex_lock(&log_array_lock);
  log_functions[num_log_functions] = f;
  log_function_params[num_log_functions] = param;
  num_log_functions++;
  pthread_mutex_unlock(&log_array_lock);
}

void msg_thread_set_timeout(int ms)
{
  log_timeout.tv_sec = ms / 1000;
  log_timeout.tv_nsec = ((long)ms % 1000L) * 1000000L;
}

/* start the logger thread with the configured log functions */
void msg_thread_start(void)
{
  pthread_create(&log_thread, 0, msg_thread, 0);
}

/* this stops the logger thread. hard. */
void msg_thread_stop(void)
{
  pthread_cancel(log_thread);
}

/* this is the main message logging thread */
void *msg_thread(void *arg)
{
  int i;
  
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, 0);
  
  while (1)
  {
    /* we use nanosleep here because nanosleep, unlike sleep and usleep, is a thread cancellation point */
    nanosleep(&log_timeout, 0);
    
    /* now walk through all log functions and call them */
    pthread_mutex_lock(&log_array_lock);
    for (i = 0; i < num_log_functions; i++)
    {
      if (log_functions[i])
	(log_functions[i])(log_function_params[i]);
    }
    pthread_mutex_unlock(&log_array_lock);
  }
}

#ifdef __cplusplus
}
#endif
