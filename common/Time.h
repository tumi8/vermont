/**
 * General functions for manipulating timing structures
 */

#if !defined(TIME_H)
#define TIME_H

#include <time.h>
#include <sys/time.h>
#include <stdint.h>



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


// The following code was shamelessly ripped from the Mash streaming media toolkit
// file ntp-time.h, and, of course, modified!
/*
 * ntp-time.h --
 *
 *      FIXME: This file needs a description here.
 *
 * Copyright (c) 1993-2002 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * A. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * B. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * C. Neither the names of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @(#) $Header: /usr/mash/src/repository/mash/mash-1/rtp/ntp-time.h,v 1.10 2003/11/19 19:20:39 aswan Exp $
 */

typedef struct {
	uint32_t lower;        /* less significant 32 bits (whole structure is in HBO!) */
	uint32_t upper;        /* more significant 32 bits */
} ntp64;


/*
 * convert microseconds to fraction of second * 2^32 (i.e., the lsw of
 * a 64-bit ntp timestamp).  This routine uses the factorization
 * 2^32/10^6 = 4096 + 256 - 1825/32 which results in a max conversion
 * error of 3 * 10^-7 and an average error of half that.
 */
inline uint32_t usec2ntp(uint32_t usec)
{
        uint32_t t = (usec * 1825) >> 5;
        return ((usec << 12) + (usec << 8) - t);
}

/*
 * Number of seconds between 1-Jan-1900 and 1-Jan-1970
 */
#define GETTIMEOFDAY_TO_NTP_OFFSET 2208988800UL

/*
 * Return a 64-bit ntp timestamp (UTC time relative to Jan 1, 1970).
 * gettimeofday conveniently gives us the correct reference -- we just
 * need to convert sec+usec to a 64-bit fixed point (with binary point
 * at bit 32).
 */
inline ntp64 ntp64time(timeval tv)
{
        ntp64 n;
        n.upper = (uint32_t)tv.tv_sec + GETTIMEOFDAY_TO_NTP_OFFSET;
        n.lower = usec2ntp((uint32_t)tv.tv_usec);
        return (n);
}

// uses same mechanism as usec2ntp, some there is some error during conversion!
inline timeval timentp64(ntp64 n)
{
	timeval tv;
	tv.tv_sec = n.upper-GETTIMEOFDAY_TO_NTP_OFFSET;
	tv.tv_usec = n.lower/4294;
    return tv;
}

inline uint32_t ntptime(timeval t)
{
        uint32_t s = (uint32_t)t.tv_sec + GETTIMEOFDAY_TO_NTP_OFFSET;
        return (s << 16 | usec2ntp((uint32_t)t.tv_usec) >> 16);
}

inline uint32_t ntptime()
{
        timeval tv;
        gettimeofday(&tv, 0);
        return (ntptime(tv));
}

inline ntp64 ntp64time()
{
        timeval tv;
        gettimeofday(&tv, 0);
        return (ntp64time(tv));
}

inline timeval unixtime()
{
        timeval tv;
        gettimeofday(&tv, 0);
        return (tv);
}

inline float ntp64_to_ms(ntp64 time)
{
	return (float)(time.upper + time.lower/4294967296.0);
}

inline float ntp32_to_ms (uint32_t time)
{
	return (float)(((time & 0xFFFF0000) >> 16) +
		((time & 0x0000FFFF) << 16)/4294967296.0);
}

inline uint32_t ntp32_substract (uint32_t x, uint32_t y)
{
	return ((x) > (y)) ? ((x) - (y)) : (((x) - (y)) + 0x7fffffff);
}



#endif
