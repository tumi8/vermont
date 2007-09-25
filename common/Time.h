/**
 * General functions for manipulating timing structures
 */

#if !defined(TIME_H)
#define TIME_H

#include <time.h>
#include <sys/time.h>



/* Subtract the `struct timeval' values X and Y,
   storing the result in RESULT.
   Return 1 if the difference is negative, otherwise 0.  */
inline int timeval_subtract(struct timeval* result, struct timeval* x, struct timeval* y)
{
  /* Perform the carry for the later subtraction by updating y. */
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (x->tv_usec - y->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;

  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}

/**
 * adds to current time the value in timediff_ms and returns the result in
 * ts
 */
inline void addToCurTime(struct timespec* ts, long timediff_ms)
{
	struct timeval tv;
	// calculate absolute time from timeout
	gettimeofday(&tv, 0);
	// add timeout value to the current time
	// if no timeout is given, use standard timeout, as we need to check the exitFlag regularly
	tv.tv_usec += timediff_ms * 1000L;
	if (tv.tv_usec >= 1000000L)
	{
		tv.tv_sec += (tv.tv_usec/1000000L);
		tv.tv_usec %= 1000000L;
	}
	ts->tv_sec = tv.tv_sec;
	ts->tv_nsec = tv.tv_usec * 1000L;
}

/**
 * compares two timespec values
 * @returns -1 if ts1 is smaller, 0 if both are equal, 1 if ts2 is smaller
 */
inline int compareTime(const struct timespec& ts1, const struct timespec& ts2)
{
	if (ts1.tv_sec < ts2.tv_sec) return -1;
	else if (ts1.tv_sec > ts2.tv_sec) return 1;
	else {
		if (ts1.tv_nsec < ts2.tv_nsec) return -1;
		else if (ts1.tv_nsec > ts2.tv_nsec) return 1;
	}
	return 0;
}

#endif
