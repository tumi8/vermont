/*
   this is vermont.
   released under GPL v2

   (C) by Ronny T. Lampert

*/
#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>
#include <sys/syslog.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <assert.h>
#include <map>
#include <stdexcept>
#include <sstream>
#include <cstring>
#include <stdlib.h>
#include "msg.h"

#ifdef __cplusplus
extern "C" {
#endif

	/** Maximum length of exception strings */
	const int EXCEPTION_MAXLEN = 1024;

	static int  syslog_mask = LOG_UPTO(LOG_WARNING);
	static bool quiet = false;
	static bool journald_enabled = false;
	static bool syslog_enabled = false;

	/*
	   we need to serialize for msg_stat()
	   just use a global lock, this isn't a contended lock
	   */
	static pthread_mutex_t stat_lock = PTHREAD_MUTEX_INITIALIZER;
	static FILE *stat_file;

	// mutex for logging function
	static pthread_mutex_t msg_mutex;

	static const char *level_to_string (int level)
	{
		switch (level) {
		case LOG_EMERG:
			return "EMERGENCY";
		case LOG_ALERT:
			return "ALERT";
		case LOG_CRIT:
			return "CRITICAL";
		case LOG_ERR:
			return "ERROR";
		case LOG_WARNING:
			return "WARNING";
		case LOG_NOTICE:
			return "NOTICE";
		case LOG_INFO:
			return "INFO";
		case LOG_DEBUG:
			return "DEBUG";
		default:
			return "UNKNOWN";
		}
	}

	/**
	 * @brief parse a string and return a logging bitmask
	 *
	 * @param arg string represent logging level
	 * @return bitmask of logging levels up to arg
	 * @return -1 if logging level is not recognised
	 */
	int
	parse_log_level (const char *arg)
	{
		if (!strcmp("debug", arg)) {
			return LOG_UPTO(LOG_DEBUG);
		} else if (!strcmp("info", arg)) {
			return LOG_UPTO(LOG_INFO);
		} else if (!strcmp("notice", arg)) {
			return LOG_UPTO(LOG_NOTICE);
		} else if (!strcmp("warning", arg)) {
			return LOG_UPTO(LOG_WARNING);
		} else if (!strcmp("err", arg)) {
			return LOG_UPTO(LOG_ERR);
		} else if (!strcmp("crit", arg)) {
			return LOG_UPTO(LOG_CRIT);
		} else if (!strcmp("alert", arg)) {
			return LOG_UPTO(LOG_ALERT);
		} else if (!strcmp("emerg", arg)) {
			return LOG_UPTO(LOG_EMERG);
		}
		return -1;
	}

	/**
	 * initializes logging system
	 * must be called at program startup!
	 */
	void msg_init()
	{
		// init the logging function's mutex
		int retval = pthread_mutex_init(&msg_mutex, 0);
		if (retval != 0) {
			fprintf(stderr, "!!! msg: pthread_mutex_init returned error code %d (%s)\n", retval, strerror(retval));
		}

		// set stdout and stderr to non-buffered
		setvbuf(stdout, NULL, _IONBF, 0);
		setvbuf(stderr, NULL, _IONBF, 0);

		if (msg_get_syslog()) {
			setlogmask(syslog_mask);
			openlog("vermont", LOG_PID, LOG_DAEMON);
		}
	}

	/**
	 * deinitializes logging function's mutex
	 */
	void msg_shutdown()
	{
		if (msg_get_syslog()) {
			closelog();
		}

		int retval = pthread_mutex_destroy(&msg_mutex);
		if (retval != 0) {
			fprintf(stderr, "!!! msg: pthread_mutex_destroy returned error code %d (%s)\n", retval, strerror(retval));
		}
	}

	/**
	 * internal function which logs given string via printf and returns the logged string in
	 * parameter logtext if it is != 0
	 */
	static void msg_intern(char* logtext, const int level, const char* fmt, va_list* args)
	{
		// we must lock via mutex, else logging outputs are mixed when several
		// threads log simultaneously
		int retval = pthread_mutex_lock(&msg_mutex);
		if (retval != 0) {
			fprintf(stderr, "!!! msg: pthread_mutex_lock returned error code %d (%s)\n", retval, strerror(retval));
		}
		struct timeval tv;
		gettimeofday(&tv, 0);
		struct tm* tform = localtime(reinterpret_cast<time_t*>(&tv.tv_sec));

#if defined(DEBUG)
		// determine thread id
		static std::map<pthread_t, int> threadids; // we want simple thread ids for logging, here is the map to do that
		static int nothreads = 0; // how many threads access this function?
		pthread_t pt = pthread_self();
		std::map<pthread_t, int>::iterator iter = threadids.find(pt);
		int threadid;
		if (iter == threadids.end()) {
			threadid = nothreads;
			threadids[pt] = nothreads++;
		} else {
			threadid = iter->second;
		}

		printf("%02d:%02d:%02d.%03ld[%d] %6s", tform->tm_hour, tform->tm_min, tform->tm_sec, tv.tv_usec/1000, threadid, level_to_string(level));
#else
		printf("%02d/%02d %02d:%02d:%02d %6s", tform->tm_mday, tform->tm_mon +1, tform->tm_hour, tform->tm_min, tform->tm_sec, level_to_string(level));
#endif
		// need helper variable here because va_list parameter of vprintf is undefined after function call
		va_list my_args;
		va_copy(my_args, *args);
		vprintf(fmt, my_args);
		va_end(my_args);
		printf("\n");

		if (logtext != NULL) {
#if defined(DEBUG)
			snprintf(logtext, EXCEPTION_MAXLEN, "%02d:%02d:%02d.%03ld[%d] %6s", tform->tm_hour, tform->tm_min, tform->tm_sec, tv.tv_usec/1000, threadid, level_to_string(level));
#else
			snprintf(logtext, EXCEPTION_MAXLEN, "%02d:%02d:%02d.%03ld %6s", tform->tm_hour, tform->tm_min, tform->tm_sec, tv.tv_usec/1000, level_to_string(level));
#endif
			vsnprintf(logtext, EXCEPTION_MAXLEN-strlen(logtext), fmt, *args);
		}
		retval = pthread_mutex_unlock(&msg_mutex);
		if (retval != 0) {
			fprintf(stderr, "!!! msg: pthread_mutex_unlock returned error code %d\n", retval);
		}
	}

	/**
	  used for internal logging
	  just outputs the given string without any additions like line numbers and so on
	  */
	static void msg_normal(const int level, const char *fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		msg_intern(NULL, level, fmt, &args);
		va_end(args);
	}

	/**
	 * expands given string with variable arguments with source file name, line and function name,
	 * if required
	 */
	static void msg_expand(char* logtext, const int line, const char* filename, const char* funcname, const char* simplefunc, const int level, const char *fmt, va_list* args)
	{
		std::stringstream fmtnew;

#if defined(PRINT_FILELOCATION)
		fmtnew << " " << filename << ":" << line;
#endif

#if defined(PRINT_WHOLEFUNCTIONNAME)
		fmtnew << " " << funcname;
#endif

#if defined(PRINT_NICELOCATION)
		fmtnew << " " << filename << "::" << simplefunc;
#endif

		fmtnew << ": " << fmt;

		msg_intern(logtext, level, fmtnew.str().c_str(), args);
	}


	/**
	 * called by macro msg which includes additional parameters for logging
	 */
	void msg2(const int line, const char* filename, const char* funcname, const char* simplefunc, const int level, const char *fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		msg_expand(NULL, line, filename, funcname, simplefunc, level, fmt, &args);
		va_end(args);
	}

	/**
	 * sets verbosity level of vermont
	 */
	void msg_setlevel(int level)
	{
		syslog_mask=level;
		setlogmask(level);
	}

	/**
	 * gets verbosity level of vermont
	 */
	int msg_getlevel()
	{
		return syslog_mask;
	}

	/**
	 * check if log to file, not to stdout/err
	 */
	void msg_setquiet(bool set_quiet)
	{
		quiet = set_quiet;
	}

	/**
	 * check if log to file, not to stdout/err
	 */
	bool msg_getquiet()
	{
		return quiet;
	}

	/**
	 * set whether to log to journald
	 */
	void msg_set_journald(bool set_journald)
	{
		journald_enabled = set_journald;
	}

	/**
	 * return true if logging to journald, false otherwise
	 */
	bool msg_get_journald()
	{
		return journald_enabled;
	}

	/**
	 * set whether to log to syslog
	 */
	void msg_set_syslog(bool set_syslog)
	{
		syslog_enabled = set_syslog;
	}

	/**
	 * return true if logging to syslog, false otherwise
	 */
	bool msg_get_syslog()
	{
		return syslog_enabled;
	}


	/**
	 * used by vermont modules to generate output statistics
	 * to use this, msg_stat_setup and msg_thread_start must be called
	 * by the main routine. This starts an extra statistics thread which calls
	 * hook functions inside different vermont modules regularly to generate
	 * statistics
	 */
	int msg_stat(const char *fmt, ...)
	{
		/* have to check if subsys is on. Else just ignore */
		if(stat_file) {
			va_list args;
			va_start(args, fmt);

			//pthread_mutex_lock(&stat_lock);
			vfprintf(stat_file, fmt, args);
			fputs("\n", stat_file);
			//pthread_mutex_unlock(&stat_lock);

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


	void vermont_assert(const char* expr, const char* description, int line, const char* filename, const char* prettyfuncname, const char* funcname)
	{
		msg_normal(LOG_ERR, "Assertion: %s", expr);
		msg_normal(LOG_ERR, "Message: %s", description);
		msg_normal(LOG_ERR, "---------------------------------------------------------------");
		msg_normal(LOG_ERR, "filename: %s:%d, function: %s (%s)", filename, line, funcname, prettyfuncname);
		exit(1);
	}


	void vermont_exception(const int line, const char* filename, const char* funcname, const char* simplefunc, const char* fmt, ...)
	{
		char text[EXCEPTION_MAXLEN+1];

		va_list args;
		va_start(args, fmt);
		msg_expand(text, line, filename, funcname, simplefunc, LOG_CRIT, fmt, &args);
		va_end(args);

#ifdef EXIT_ON_EXCEPTION
		exit(0);
#else
		throw std::runtime_error(text);
#endif
	}

#ifdef __cplusplus
}
#endif

