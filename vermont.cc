/*
 this is vermont.
 released under GPL v2

 (C) by Ronny T. Lampert

 */
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#include "iniparser.h"
#include "msg.h"
#include "subsystems.h"

static void usage();
static void sig_handler(int x);
static int setup_signal(int signal, void (*handler)(int));
int vermont_readconf(char *file, dictionary **conf);

/* initialized subsystems */
unsigned int v_subsystems;


int main(int ac, char **dc)
{
	dictionary *config;
	int c, debug_level=MSG_DEFAULT;
        char *config_file=NULL;

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

	if(vermont_readconf(config_file, &config)) {
                exit(-1);
	}

	subsys_dump(v_subsystems);

	iniparser_dump(config, stdout);

        return 0;
}


/*
 read the config from *file and attach the iniparser stuff to **conf
 perform basic checks
 */
int vermont_readconf(char *file, dictionary **conf)
{
        dictionary *d;

	/* read configuration */
	d=iniparser_new(file);
	if(!d) {
		msg(MSG_FATAL, "could not open config_file %s", file);
                return(-1);
	}

        /* check if all section we need are present */
	if((iniparser_find_entry(d, "collector") == 1) &&
	   (iniparser_find_entry(d, "sampler") == 1) &&
	   (iniparser_find_entry(d, "main") == 1)
	  ) {
		subsys_on(&v_subsystems, SUBSYS_CONFIG);
		*conf=d;

		return 0;

	} else {
		msg(MSG_FATAL, "not all needed sections in config %s", file);

		return -1;
	}
}


/* bla bla bla */
static void usage()
{
	printf(
	       "Versatile MONitoring Tool - VERMONT\n" \
	       " mandatory:
	       "    -f <inifile>     load config\n" \
	       " optional:
	       "    -d               increase debug level\n" \
	      );
}


static int setup_signal(int signal, void (*handler)(int))
{
	struct sigaction sig;

	sig.sa_handler=sig_handler;
        sig.sa_flags=SA_RESTART;
        sigemptyset(&sig.sa_mask);
        return (sigaction(signal, &sig, NULL));
}


/* just shallow right now */
static void sig_handler(int x)
{
        msg(MSG_DIALOG, "got signal %d", x);
}
