/*
 this is vermont.
 released under GPL v2

 (C) by Ronny T. Lampert

 */
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <vector>

/* 
 foreign subsystems: sampler
 */
#include "ipfixlolib/ipfixlolib.h"

/*
#include "sampler/Packet.h"
#include "sampler/Filter.h"
#include "sampler/Observer.h"
#include "sampler/PacketSink.h"
#include "sampler/ExporterSink.h"
#include "sampler/PacketProcessor.h"
#include "sampler/Template.h"
#include "sampler/IPHeaderFilter.h"
 */

/* collector */


/* own systems */
#include "iniparser.h"
#include "msg.h"
#include "subsystems.h"
#include "vermont.h"
#include "config_sampler.h"

using namespace std;

static void usage();
static void sig_handler(int x);
static int setup_signal(int signal, void (*handler)(int));
static int vermont_readconf(dictionary **conf, char *file);
static int vermont_configure(struct v_objects *v);

int main(int ac, char **dc)
{
        dictionary *config;
	int c, debug_level=MSG_DEFAULT;
        char *config_file=NULL;

        struct v_objects v_objects;

	/* parse command line */
	while((c=getopt(ac, dc, "hf:d")) != -1) {

		switch(c) {

		case 'f':
			config_file=optarg;
			break;

		case 'd':
			debug_level++;
                        break;

		case 'h':
			usage();
                        return 0;

		default:
			usage();
                        break;
		}
	}

	/* setup verboseness */
	msg_setlevel(debug_level);

	setup_signal(SIGINT, sig_handler);

        if(vermont_readconf(&config, config_file)) {
		exit(-1);
	}

	v_objects.v_config=config;

	if(vermont_configure(&v_objects)) {
                exit(-1);
        }

        subsys_dump(v_objects.v_subsystems);

        return 0;
}


/*
 read the config from *file and attach the iniparser stuff to **conf
 perform basic checks

 NOTE: the funny thing is, all returned strings are direct pointer into
 the dictionary-struct and as such valid until the dictionary is
 free()'d again.

 SO WE USUALLY DONT COPY!
 */
static int vermont_readconf(dictionary **conf, char *file)
{
        dictionary *d;

	/* read configuration */
	if(!(d=iniparser_new(file))) {
		msg(MSG_FATAL, "could not open config_file %s", file);
                return(-1);
	}

        /* check if all section we need are present */
	if((iniparser_find_entry(d, "collector") == 1) &&
	   (iniparser_find_entry(d, "sampler") == 1) &&
	   (iniparser_find_entry(d, "main") == 1)
	  ) {
		*conf=d;

		return 0;

	} else {
		msg(MSG_FATAL, "Config: not all needed sections in config %s", file);

		return -1;
	}
}


/*
 this is the main configuration entry point
 configure all subsystems peu a peu
 */
static int vermont_configure(struct v_objects *v)
{
        dictionary *conf=v->v_config;

	/* if the sampler is not needed, interface will say "off" */
	char *run_sampler=iniparser_getvalue(conf, "sampler", "interface");
	char *run_concentrator=iniparser_getvalue(conf, "concentrator", "listen_ip");
        char *hooking=iniparser_getvalue(conf, "main", "packets");

	/*
	 check if we run the sampler
	 if sampler is off, then we can use the sampler->concentrator hook
         */
	if(strcasecmp(run_sampler, "off") == 0) {
		if(strcasecmp(hooking, "off") != 0) {
			msg(MSG_FATAL, "sampler input is disabled, but hooking is used");
                        return -1;
		}

		msg(MSG_DIALOG, "not running sampler subsystem");
	} else {
                if(!configure_sampler(v)) {
                        msg(MSG_FATAL, "Main: Could not configure the sampler");
                        return -1;
                }

	}

        if(strcasecmp(run_concentrator, "off") == 0) {
                msg(MSG_DIALOG, "not running concentrator subsystem");
        } else {
                /*
                if(!configure_concentrator(v)) {
                	msg(MSG_FATAL, "Main: Could not configure the concentrator");
                        return -1;
                }
                */
        }


        return 0;
}


/* bla bla bla */
static void usage()
{
	printf(
	       "VERsatile MONitoring Tool - VERMONT\n" \
	       " mandatory:\n" \
	       "    -f <inifile>     load config\n" \
	       " optional:\n" \
	       "    -d               increase debug level\n" \
	      );
}


static int setup_signal(int signal, void (*handler)(int))
{
	struct sigaction sig;

	sig.sa_handler=sig_handler;
        sig.sa_flags=SA_RESTART;
        sigemptyset(&sig.sa_mask);
        return(sigaction(signal, &sig, NULL));
}


/* just shallow right now */
static void sig_handler(int x)
{
	msg(MSG_DIALOG, "got signal %d - exiting", x);
	exit(2);
}


static void __cplusplus_really_sucks_andPeopleUsingVeryStrangeNamingConventionsWithLongLongExplicitBlaBla(){};
