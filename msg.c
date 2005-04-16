/*
 this is vermont.
 released under GPL v2

 (C) by Ronny T. Lampert

 */
#include <stdio.h>
#include <stdarg.h>
#include "msg.h"

#ifdef __cplusplus
extern "C" {
#endif

static int msg_level=MSG_DEFAULT;
static char *MSG_TAB[]={ "FATAL  ", "VERMONT", "ERROR  ", "DEBUG  ", "INFO   ", 0};

void msg(int level, char *fmt, ...)
{
	/* nummerically higher value means lower priority */
	if (level > msg_level) {
		return;
	} else {
		va_list args;
		printf("%s: ", MSG_TAB[level]);
		va_start(args, fmt);
		vprintf(fmt, args);
		va_end(args);
		printf("\n");
	}
}


void msg_setlevel(int level)
{
        msg_level=level;
}

#ifdef __cplusplus
}
#endif
