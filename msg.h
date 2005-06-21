/*
 this is vermont.
 released under GPL v2

 (C) by Ronny T. Lampert

 */
#ifndef MSG_H
#define MSG_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* function prototype for message logging */  
typedef void (*LOGFUNCTION)(void *);

/* the maximum number of functions that will be called by the message logger thread */
#define MAX_LOG_FUNCTIONS 256
  
/* well-know DPRINTF() - will be left out if DEBUG not set */
#ifdef DEBUG
#define DPRINTF(fmt, args...) printf("-DBG- " fmt, ##args)
#else
#define DPRINTF(fmt, args...)
#endif

/* define for setting up the msg system */
#define MSG_SETUP_NEW 1

/* defines for the message system */
#define MSG_BLANK 256
#define MSG_INFO 4
#define MSG_DEBUG 3
#define MSG_ERROR 2
#define MSG_DIALOG 1
#define MSG_FATAL 0
#define MSG_DEFAULT MSG_ERROR

void msg(int, char *, ...);
void msg_setlevel(int);
int msg_stat(char *fmt, ...);
int msg_stat_setup(int mode, FILE *f);

/* msg_thread functions for the timed logger thread */
void msg_thread_add_log_function(LOGFUNCTION f, void *param);
void msg_thread_set_timeout(int ms);
void msg_thread_start();
void msg_thread_stop();
void *msg_thread(void *);

#ifdef __cplusplus
}
#endif

#endif
