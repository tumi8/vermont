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

/* function names and source file locations are only to be printed with log messages during debug mode */
#ifdef DEBUG

	/** should function names including parameter and return types be printed in logging messages? */
	//#define PRINT_WHOLEFUNCTIONNAME

	/** should source file name and line numbers be printed in logging messages? */
	//#define PRINT_FILELOCATION

	/** print 'short version' of class + function name by printing filename:function name */
	#define PRINT_NICELOCATION

#endif

/* defines for the message system */
#define MSG_BLANK 256
#define MSG_VDEBUG 5   // mostly for ipfix byte-level messages
#define MSG_DEBUG 4    // debugging messages, for example used by DPRINTF
#define MSG_INFO 3     // informational messages, shown without debug-mode but only with verbose logging enabled
#define MSG_ERROR 2    // error or warning messages which are shown during default execution
#define MSG_DIALOG 1   // messages which are shown during default execution
#define MSG_FATAL 0    // fatal messages which are shown every time
//#define MSG_DEFAULT MSG_ERROR


void msg_init(void);
void msg_shutdown(void);
void msg2(const int, const char*, const char*, const char*, const int, const char *, ...);
void msg_setlevel(int);
int msg_stat(const char *fmt, ...);
int msg_stat_setup(int mode, FILE *f);
void vermont_assert(const char* expr, const char* description, int line, const char* filename, const char* prettyfuncname, const char* funcname);
void vermont_exception(const int, const char*, const char*, const char*, const char*, ...);

/* msg_thread functions for the timed logger thread */
//int msg_thread_add_log_function(LOGFUNCTION f, void *param);
//void msg_thread_set_timeout(int ms);
//int msg_thread_start();
//int msg_thread_stop();
//void * msg_thread(void *);

//#if !defined(__PRETTY_FUNCTION__)
	//#define __PRETTY_FUNCTION__ "<unknown>"
//#endif

// useful defines for logging
#define THROWEXCEPTION(fmt, args...) vermont_exception(__LINE__, __FILE__, __PRETTY_FUNCTION__, __func__, fmt, ##args)

#define msg(lvl, fmt, args...) msg2(__LINE__, __FILE__, __PRETTY_FUNCTION__, __func__, lvl, fmt, ##args)

#ifdef DEBUG

#define DPRINTF(fmt, args...) msg2(__LINE__, __FILE__, __PRETTY_FUNCTION__, __func__, MSG_DEBUG, fmt, ##args)
#define DPRINTFL(lvl, fmt, args...) msg2(__LINE__, __FILE__, __PRETTY_FUNCTION__, __func__, lvl, fmt, ##args)

#define ASSERT(exp, description)                                                                        \
    {                                                                                                   \
        if (!(exp)) {                                                                                   \
	    vermont_assert(#exp, (description), __LINE__, __FILE__, __PRETTY_FUNCTION__, __func__);     	\
        }                                                                                               \
    }


#else

#define DPRINTF(fmt, args...)
#define DPRINTFL(lvl, fmt, args...)
#define ASSERT(exp, description)

#endif

#ifdef __cplusplus
}
#endif

#endif
