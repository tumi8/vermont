/*
 this is vermont.
 released under GPL v2

 (C) by Ronny T. Lampert

 */
#ifndef MSG_H
#define MSG_H

#define __STDC_FORMAT_MACROS 1
#include <inttypes.h>
#include <stdio.h>
#include <stdbool.h>
#include <syslog.h>
#ifdef JOURNALD_SUPPORT_ENABLED
#include <systemd/sd-journal.h>
#else
#define sd_journal_print(lvl, ...)
#endif

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
	#define PRINT_FILELOCATION

	/** print 'short version' of class + function name by printing filename:function name */
	//#define PRINT_NICELOCATION

#else
	//#define PRINT_WHOLEFUNCTIONNAME
#endif

void msg_init(void);
void msg_shutdown(void);
void msg2(const int, const char*, const char*, const char*, const int, const char *, ...);
int parse_log_level (const char *arg);
void msg_setlevel(int);
int msg_getlevel();
void msg_setquiet(bool);
bool msg_getquiet();
void msg_set_journald(bool);
bool msg_get_journald();
void msg_set_syslog(bool);
bool msg_get_syslog();
int msg_stat(const char *fmt, ...);
int msg_stat_setup(int mode, FILE *f);

#ifndef __has_feature
#define __has_feature(x) 0
#endif
#ifndef CLANG_ANALYZER_NORETURN
#if __has_feature(attribute_analyzer_noreturn)
#define CLANG_ANALYZER_NORETURN __attribute__((analyzer_noreturn))
#else
#define CLANG_ANALYZER_NORETURN
#endif
#endif

void vermont_assert(const char* expr, const char* description, int line, const char* filename, const char* prettyfuncname, const char* funcname) __attribute__((__noreturn__));
void vermont_exception(const int, const char*, const char*, const char*, const char*, ...) CLANG_ANALYZER_NORETURN;

//#if !defined(__PRETTY_FUNCTION__)
	//#define __PRETTY_FUNCTION__ "<unknown>"
//#endif

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif
// useful defines for logging
#define THROWEXCEPTION(...) \
	__extension__ \
	({ \
		if (msg_getlevel() & LOG_MASK(LOG_CRIT)) { \
			if (msg_get_syslog()) { \
				syslog(LOG_CRIT, __VA_ARGS__); \
			} \
			if (msg_get_journald()) { \
				sd_journal_print(LOG_CRIT, __VA_ARGS__); \
			} \
		} \
		vermont_exception(__LINE__, __FILE__, __PRETTY_FUNCTION__, __func__, __VA_ARGS__); \
	})

#define msg(...) _msg_helper(__VA_ARGS__)
#define _msg_helper(lvl, ...) \
	__extension__ \
	({ \
		if (msg_getlevel() & LOG_MASK(lvl)) { \
			if (msg_get_syslog()) { \
				syslog(lvl, __VA_ARGS__); \
			} \
			if (msg_get_journald()) { \
				sd_journal_print(lvl, __VA_ARGS__); \
			} \
			if (!msg_getquiet()) { \
				msg2(__LINE__, __FILE__, __PRETTY_FUNCTION__, __func__, lvl, __VA_ARGS__); \
			} \
		} \
	})
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#ifdef DEBUG

#define DPRINTF_INFO(...) msg(LOG_INFO, __VA_ARGS__)
#define DPRINTF_DEBUG(...) msg(LOG_DEBUG, __VA_ARGS__)

#define ASSERT(exp, description)                                                                        \
    __extension__                                                                                       \
    ({                                                                                                  \
        if (!(exp)) {                                                                                   \
            if (msg_getlevel() & LOG_MASK(LOG_ERR)) {                                                 \
                if (msg_get_syslog()) {                                                                 \
                    syslog(LOG_ERR, description);                                                     \
                }                                                                                       \
                if (msg_get_journald()) {                                                               \
                    sd_journal_print(LOG_ERR, description);                                           \
                }                                                                                       \
            }                                                                                           \
	    vermont_assert(#exp, (description), __LINE__, __FILE__, __PRETTY_FUNCTION__, __func__);     	\
        }                                                                                               \
    })


#else

#define DPRINTF_INFO(...)
#define DPRINTF_DEBUG(...)
#define ASSERT(exp, description)

#endif

#ifdef __cplusplus
}
#endif

#endif
