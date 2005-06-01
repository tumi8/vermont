/*
 uri parser
 Copyright 1998 ams@wiw.org (Abhijit Menon-Sen)

 changes by Ronny T. Lampert, 2005 to suit vermont
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifndef URI_H
#define URI_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	/* http://host:port/path?query#frag */
	char *abs;
	/* http, ftp, ... */
	char *scheme;
	/* host specification */
	char *host;
	/* /path?query */
	char *rel;
	/* #frag */
	char *frag;

	/* port */
	uint16_t  port;
} uri;

uri * uri_parse(char *r);
void uri_free(uri *u);
char *uri_abs(uri *u);

#ifdef __cplusplus
}
#endif

#endif
