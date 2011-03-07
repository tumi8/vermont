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
	int retval = semaphore_timedwait(*sem, mts);
	switch (retval) {
	case KERN_SUCCESS:
		return 0;
	case KERN_OPERATION_TIMED_OUT:
		return ETIMEDOUT;
	case KERN_ABORTED:
		return EINTR;
	default:
		msg(MSG_FATAL, "Got unknown mach_semaphore condition %d (%s)", retval, mach_error_string(retval));
		return EINVAL;
	}
}

#endif
