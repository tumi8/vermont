#ifdef __APPLE__

#include "sem_timedwait.h"
#include <errno.h>
#include <common/msg.h>
#include <time.h>

// Mac OS X timedwait wrapper
int sem_timedwait_mach(semaphore_t* sem, const struct timespec* ts) {
	mach_timespec_t mts;
	mts.tv_sec = ts->tv_sec;
	mts.tv_nsec = ts->tv_nsec;
	//msg(MSG_FATAL, "Waiting for %lu sec %lu nsec", ts->tv_sec, ts->tv_nsec);
	int retval = semaphore_timedwait(*sem, mts);
	switch (retval) {
	case KERN_SUCCESS:
		return 0;
	case KERN_OPERATION_TIMED_OUT:
		errno = ETIMEDOUT;
		//msg(MSG_FATAL, "Got a timeout: %lu %lu", ts->tv_sec, ts->tv_nsec);
		break;
	case KERN_ABORTED:
		errno = EINTR;
		//msg(MSG_FATAL, "interrupted");
		break;
	default:
		//msg(MSG_FATAL, "Got unknown mach_semaphore condition %d (%s)", retval, mach_error_string(retval));
		errno =  EINVAL;
		break;
	}
	return -1;
}

#endif
