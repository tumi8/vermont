/*
 uri parser
 Copyright 1998 ams@wiw.org (Abhijit Menon-Sen)

 changes by Ronny T. Lampert, 2005 to suit vermont
 fixes over fixes
 make uri_parse return allocated pointer or NULL

 */

#include "uri.h"

/*
 * uri_parse() breaks a string into a uri and uri_abs() does the opposite.
 *
 * see RFC 2396: "Uniform Resource Identifiers (URI): Generic Syntax"
 * this is not as simple as i make it look.
 */

/* http: */
static int scheme(uri *u, char *str)
{
	char *c = str;
	int  len;

	/*
	 RFC 2396, 3.1: Scheme Component
	 scheme        = alpha *( alpha | digit | "+" | "-" | "." )
	 */

        if(!isalpha(*c)) {
                return (-1);
        }

        while (isalnum(*c) || *c == '+' || *c == '-' || *c == '.') {
                c++;
        }

	len = c - str;

	/* bad scheme */
        if(*c != ':') {
                return (-1);
        }

        u->scheme = (char *)malloc(len+1);
        strncpy(u->scheme, str, len);

        /*
         freeking terminate the FUCKING thing because strncpy DOES NOT ALWAYS!
         man-page:

         "Thus, if there is no null byte among the first n bytes
         of src, the result will not be null-terminated."

         */
        u->scheme[len]=0;

	return(len+1);
}

/* //foo.bar:123 */
static int netloc(uri *u, char *str)
{
	char *c = str, *d;
	int  len, port = 0, pre = 0;

	/*
	 RFC 2396, 3.2.2: Server-based Naming Authority
	 server        = [ [ userinfo "@" ] hostport ]
	 hostport      = host [ ":" port ]
	 port          = *digit
	 we ignore userinfo, and don't validate host for now.
	 */

	while (*c == '/') {
		c++;
		pre++;
	}

	while (*c && *c != '/' && *c != ' ' && *c != '\n') {
		c++;
	}

	/* empty netloc */
	if((len = c - (str + pre)) < 1) {
		return (-1);
	}

	d = (char *)malloc(len);
	strncpy(d, str+pre, len);

	if ((c = strchr(d, ':')) != NULL) {
		port = atoi(c+1);
		u->host = (char *)malloc(c - d);
		strncpy(u->host, d, c - d);
		free(d);
	}
	else {
		u->host = d;
	}
	u->port = port ? port : 80;

	return (pre + len);
}

/* /baz?quux */
static int pathq(uri *u, char *str)
{
	char *c = str;
	int  len;

	/*
	 RFC 2396, 3.4: Query Component
	 "The query component is a string of information to be interpreted by
	 the resource."
	 query         = *uric
	 we ignore reserved characters for now.
	 */

	while (*c && *c != '#' && *c != ' ' && *c != '\n') { c++; }

	len = c - str;

	if (len < 1) {
		/* no path/query */
		u->rel  = (char *)malloc(2);
		strcpy(u->rel, "/");
	}
	else {
		u->rel = (char *)malloc(len + 1);
		strncpy(u->rel, str, len);
		u->rel[len] = '\0';
	}

	return (len);
}

/* #frag */
static int frag(uri *u, char *str)
{
	char *c = str, *d;
	int  len;

	/*
	 RFC 2396, 4.1: Fragment Identifier
	 fragment      = *uric
	 */

	if (*c++ != '#') {
		u->frag = NULL;
		return (0);
	}

	d = c;
	while (*c && *c != ' ' && *c != '\n') { c++; }

	len = c - d;

	if (len < 1) {
		u->frag = NULL;
	}
	else {
		u->frag = (char *)malloc(len + 1);
		strncpy(u->frag, d, len);
		u->frag[len] = '\0';
	}

	return (len);
}

/*
 parse something like: http://foo.bar:123/baz?quux#frag
 return newly allocated uri

 call uri_free(u) afterwards to free
 */
uri * uri_parse(char *r)
{
        uri *u;
        char *c;
        int  i;

        if(!(u=malloc(sizeof(uri)))) {
                goto out;
        }

        if(!(u->abs = strdup(r))) {
                goto out1;
        }

	c = u->abs;
        while (*c == ' ') {
                c++;
        }

        if ((i = scheme(u, c)) < 0) {
                goto out1;
        }
	c += i;

        if ((i = netloc(u, c)) < 0) {
                goto out1;
        }
	c += i;

        if ((i =  pathq(u, c)) < 0) {
                goto out1;
        }
	c += i;

        if ((i =   frag(u, c)) < 0) {
                goto out1;
        }
        c += i;

        return(u);

out1:
        uri_free(u);
out:
        return NULL;
}


/* call that to free uri */
void uri_free(uri *u)
{
        if(u) {
                if(u->abs) free(u->abs);
                if(u->scheme) free(u->scheme);
                if(u->host) free(u->host);
                if(u->rel) free(u->rel);
                if(u->frag) free(u->frag);

                free(u);
        }
}


/*
 reassemble http, foo.bar, 123, /baz?quux, frag  into URL

 returned string is allocated, you have to free
 */
char *uri_abs(uri *u)
{
	char *str, port[16];

	snprintf(port, 16, u->port == 80 ? "" : ":%d", u->port);

	str = (char *)malloc(3 + strlen(u->scheme)
			     + strlen(u->host)
			     + strlen(port)
			     + strlen(u->rel));
	/* + u->frag == NULL ? 0 : 1 + strlen(u->frag)); */
	strcat(str, u->scheme);
	strcat(str, "://");
	strcat(str, u->host);
	strcat(str, port);
	strcat(str, u->rel);

	/*
	 RFC 2396, 4.1: Fragment Identifier
	 "As such, it is not part of a URI, but is often used in conjunction
	 with a URI."

	 if (u->frag != NULL) {
	 strcat(str, "#");
	 strcat(str, u->frag);
	 }
	 */

	return (str);
}
