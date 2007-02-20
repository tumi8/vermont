/*
 this is vermont.
 released under GPL v2

 (C) by Ronny T. Lampert
 (C) by Lothar Braun <mail@lobraun.de>

 */
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* own systems */
#include "msg.h"
#include "ipfix_configuration.h"

using namespace std;

static void usage();
static void sig_handler(int x);
static int setup_signal(int signal, void (*handler)(int));

IpfixConfiguration* ipfixConfig = NULL;

int main(int ac, char **dc)
{
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
                default:
                        /* print usage and quit vermont, if unknow switch */
                        usage();
                        exit(1);
                }
        }
	
	if (config_file == NULL) {
		msg(MSG_FATAL, "no config file given, but mandatory");
		usage();
		return -1;	
	}

	/* setup verboseness */
        msg_setlevel(debug_level);

	setup_signal(SIGINT, sig_handler);
	
	
	
	try {
		ipfixConfig = new IpfixConfiguration(config_file);
	        ipfixConfig->readSubsystemConfiguration();
		ipfixConfig->connectSubsystems();
		ipfixConfig->startSubsystems();
	} catch (std::exception& e) {
		msg(MSG_FATAL, "%s", e.what());
		delete ipfixConfig;
		return -1;
	}

	sleep(1);

	time_t t = time(NULL);
	msg(MSG_DIALOG, "up and running at %s", ctime(&t));
	
	ipfixConfig->pollAggregatorLoop();
	
	delete ipfixConfig;
	
	return 0;
}


/* bla bla bla */
static void usage()
{
	printf(
			"VERsatile MONitoring Tool - VERMONT\n" \
			" mandatory:\n" \
			"    -f <xmlfile>     load config\n" \
			" optional:\n" \
			"    -d               increase debug level (specify multiple for even more)\n" \
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
    static bool shutdownInitiated = false;

    if (shutdownInitiated) {
        msg(MSG_DIALOG, "second signal received, shutting down the hard way!");
        exit(2);
    }

    shutdownInitiated = true;
	msg(MSG_DIALOG, "got signal %d - exiting", x);

    if (ipfixConfig) {
        delete ipfixConfig;
    }

	// TODO: maybe there are more constructors to be called?
	exit(2);
}


//static void __cplusplus_really_sucks_andPeopleUsingVeryStrangeNamingConventionsWithLongLongExplicitBlaBlaAndfUnNycasE()
//{
	/*
	The Paris crew at MOME Interop 2005

	Christoph "C'est bon" "Das koennte beruehmt sein" Sommer
	Lothar "Pragmatic" "Ai verdammt, das funktioniert nicht" Braun
	Ronny T. "Agent Provocateur" "Le (SVN-)depot, c'est moi" Lampert


	Thanks to

	Falko "Ich hab da mal so ne Idee" Dressler
	Gerhard "Man muesste mal" Muenz

	who made it possible to fly to Paris.
	*/
//};
