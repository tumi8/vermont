/*
   this is vermont.
   released under GPL v2

   (C) by Ronny T. Lampert

*/
#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>
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

	/* the maximum number of functions that will be called by the message logger thread */
	const int MAX_LOG_FUNCTIONS = 256;

	/** Maximum length of exception strings */
	const int EXCEPTION_MAXLEN = 1024;

	static int msg_level=MSG_ERROR;
	static const char *MSG_TAB[]={ "FATAL  ", "VERMONT", "ERROR  ", "INFO   ", "DEBUG  ", "VDEBUG ", 0};

	/*
	   we need to serialize for msg_stat()
	   just use a global lock, this isn't a contended lock
	   */
	static pthread_mutex_t stat_lock = PTHREAD_MUTEX_INITIALIZER;
	static FILE *stat_file;

	/* the log functions which the message logger thread calls and stuff needed for them */
	static LOGFUNCTION log_functions[MAX_LOG_FUNCTIONS];
	static void *log_function_params[MAX_LOG_FUNCTIONS];
	static int num_log_functions;

	/* each log_timeout, the logger thread will call all registered logging functions */
	static struct timespec log_timeout = {
		300, 0
	};
	//static pthread_t log_thread;

	// mutex for logging function
	static pthread_mutex_t msg_mutex;

	/**
	 * initializes logging system
	 * must be called at program startup!
	 */
	void msg_init()
	{
		// init the logging function's mutex
		int retval = pthread_mutex_init(&msg_mutex, 0);
		if (retval != 0) {
			printf("!!! msg: pthread_mutex_init returned error code %d (%s)\n", retval, strerror(retval));
		}

		// set stdout and stderr to non-buffered
		setvbuf(stdout, NULL, _IONBF, 0);
		setvbuf(stderr, NULL, _IONBF, 0);
	}

	/**
	 * deinitializes logging function's mutex
	 * vermont does not call this function
	 */
	void msg_shutdown()
	{
		pthread_mutex_destroy(&msg_mutex);
	}

	/**
	 * internal function which logs given string via printf and returns the logged string in
	 * parameter logtext if it is != 0
	 */
	void msg_intern(char* logtext, const int level, const char* fmt, va_list* args)
	{
#if defined(DEBUG)
		static std::map<pthread_t, int> threadids; // we want simple thread ids for logging, here is the map to do that
		static int nothreads = 0; // how many threads access this function?
#endif

		/* nummerically higher value means lower priority */
		if (level > msg_level) {
			return;
		} else {
			// we must lock via mutex, else logging outputs are mixed when several
			// threads log simultaneously
			int retval = pthread_mutex_lock(&msg_mutex);
			if (retval != 0) printf("!!! msg: pthread_mutex_lock returned error code %d (%s)\n", retval, strerror(retval));
			struct timeval tv;
			gettimeofday(&tv, 0);
			struct tm* tform = localtime(reinterpret_cast<time_t*>(&tv.tv_sec));

#if defined(DEBUG)
			// determine thread id
			pthread_t pt = pthread_self();
			std::map<pthread_t, int>::iterator iter = threadids.find(pt);
			int threadid;
			if (iter == threadids.end()) {
				threadid = nothreads;
				threadids[pt] = nothreads++;
			} else {
				threadid = iter->second;
			}

			printf("%02d:%02d:%02d.%03ld[%d] %6s", tform->tm_hour, tform->tm_min, tform->tm_sec, tv.tv_usec/1000, threadid, MSG_TAB[level]);
#else
			//printf("%02d:%02d:%02d.%03ld %6s", tform->tm_hour, tform->tm_min, tform->tm_sec, tv.tv_usec/1000, MSG_TAB[level]);
			// Gerhard: message level is more important than Milliseconds (at least to me)
			printf("%02d/%02d %02d:%02d:%02d %6s", tform->tm_mday, tform->tm_mon +1, tform->tm_hour, tform->tm_min, tform->tm_sec, MSG_TAB[level]);
#endif
			// need helper variable here because va_list parameter of vprintf is undefined after function call
			va_list my_args;
			va_copy(my_args, *args);
			vprintf(fmt, my_args);
			printf("\n");

			if (logtext != NULL) {
#if defined(DEBUG)
				snprintf(logtext, EXCEPTION_MAXLEN, "%02d:%02d:%02d.%03ld[%d] %6s", tform->tm_hour, tform->tm_min, tform->tm_sec, tv.tv_usec/1000, threadid, MSG_TAB[level]);
#else
				snprintf(logtext, EXCEPTION_MAXLEN, "%02d:%02d:%02d.%03ld %6s", tform->tm_hour, tform->tm_min, tform->tm_sec, tv.tv_usec/1000, MSG_TAB[level]);
#endif
				vsnprintf(logtext, EXCEPTION_MAXLEN-strlen(logtext), fmt, *args);
			}
			retval = pthread_mutex_unlock(&msg_mutex);
			if (retval != 0) printf("!!! msg: pthread_mutex_unlock returned error code %d\n", retval);
		}
	}

	/**
	  used for internal logging
	  just outputs the given string without any additions like line numbers and so on
	  */
	void msg_normal(const int level, const char *fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		msg_intern(0, level, fmt, &args);
		va_end(args);
	}

	/**
	 * expands given string with variable arguments with source file name, line and function name,
	 * if required
	 */
	void msg_expand(char* logtext, const int line, const char* filename, const char* funcname, const char* simplefunc, const int level, const char *fmt, va_list* args)
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
		msg_expand(0, line, filename, funcname, simplefunc, level, fmt, &args);
		va_end(args);
	}

	/**
	 * sets verbosity level of vermont
	 */
	void msg_setlevel(int level)
	{
		msg_level=level;
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


	int msg_thread_add_log_function(LOGFUNCTION f, void *param)
	{
		int ret;

		pthread_mutex_lock(&stat_lock);
		if(num_log_functions < MAX_LOG_FUNCTIONS) {
			log_functions[num_log_functions] = f;
			log_function_params[num_log_functions] = param;
			num_log_functions++;
			ret=0;
		} else {
			ret=1;
		}
		pthread_mutex_unlock(&stat_lock);

		return(ret);
	}


	void msg_thread_set_timeout(int ms)
	{
		assert(ms > 0);
		log_timeout.tv_sec = ms / 1000;
		log_timeout.tv_nsec = ((long)ms % 1000L) * 1000000L;
	}

	void vermont_assert(const char* expr, const char* description, int line, const char* filename, const char* prettyfuncname, const char* funcname)
	{
		msg_normal(MSG_ERROR, "Assertion: %s", expr);
		msg_normal(MSG_ERROR, "Message: %s", description);
		msg_normal(MSG_ERROR, "---------------------------------------------------------------");
		msg_normal(MSG_ERROR, "filename: %s:%d, function: %s (%s)", filename, line, funcname, prettyfuncname);
		exit(1);
	}


	void vermont_exception(const int line, const char* filename, const char* funcname, const char* simplefunc, const char* fmt, ...)
	{
		char text[EXCEPTION_MAXLEN+1];

		va_list args;
		va_start(args, fmt);
		msg_expand(text, line, filename, funcname, simplefunc, MSG_FATAL, fmt, &args);
		va_end(args);

		throw std::runtime_error(text);
	}


	/* start the logger thread with the configured log functions */
	/*int msg_thread_start(void)
	  {
	  return(pthread_create(&log_thread, NULL, msg_thread, NULL));
	  }*/


	/* this stops the logger thread. hard. */
	/*int msg_thread_stop(void)
	  {
	  return(pthread_cancel(log_thread));
	  }*/


	/* this is the main message logging thread */
	/*void * msg_thread(void *arg)
	  {
	  int i;

	  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	  while (1) {

	// we use nanosleep here because nanosleep
	// unlike sleep and usleep, is a thread cancellation point

	nanosleep(&log_timeout, NULL);

	// now walk through all log functions and call them
	pthread_mutex_lock(&stat_lock);

	msg_stat("Vermont: Beginning statistics dump at %u", time(NULL));
	for(i=0; i < num_log_functions; i++) {
	if(log_functions[i]) {
	(log_functions[i])(log_function_params[i]);
	}
	}
	// append \n to get one empty line between following dumps
	msg_stat("Vermont: Statistics dump finished at %u\n", time(NULL));

	pthread_mutex_unlock(&stat_lock);
	}

	return 0;
	}*/

#ifdef __cplusplus
}
#endif

