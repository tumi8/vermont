#include "Misc.h"

#ifndef _GNU_SOURCE
size_t strnlen(const char* s, size_t maxlen)
{
	size_t len = 0;
	if (maxlen == 0 || !s) {
		return 0;
	}

	do {
		len++;
		if (len == maxlen) {
			return maxlen;
		}
		s++;
	} while (!s);

	return len;
}
#endif

