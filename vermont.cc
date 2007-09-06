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
#include "ipfix_configuration.h"

#include "common/TimeoutSemaphore.h"
#include "common/msg.h"
#include "common/StatisticsManager.h"

#include "cfg/ConfigManager.h"

using namespace std;

static void usage();
static void sig_INT_handler(int x);
static void sig_USR1_handler(int x);

static int setup_signal(int signal, void (*handler)(int));

IpfixConfiguration* ipfixConfig = NULL;
ConfigManager manager;

int main(int ac, char **dc)
{

	int c, debug_level=MSG_DEFAULT;
	char *config_file=NULL;
	uint32_t statInterval = 0;
	string statFile = "stats.log";

	msg_init();

	/* parse command line */
	while ((c=getopt(ac, dc, "hf:ds:u:")) != -1) {

		switch (c) {

		case 'f':
			config_file=optarg;
			break;

		case 'd':
			debug_level++;
			break;

		case 's':
			statInterval = atoi(optarg);
			break;

		case 'u':
			statFile = optarg;

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

	if (statInterval>0) {
		msg(MSG_DIALOG, "enabling statistics output to file %s using interval %dms", statFile.c_str(), statInterval);
		StatisticsManager::getInstance().setInterval(statInterval);
		StatisticsManager::getInstance().setOutput(statFile);
		StatisticsManager::getInstance().start();
	}


	/* setup verboseness */
	msg(MSG_DEFAULT, "message debug level is %d", debug_level);
	msg_setlevel(debug_level);

	setup_signal(SIGINT, sig_INT_handler);
	setup_signal(SIGUSR1, sig_USR1_handler);
	
	manager.parseConfig(string(config_file));
/*
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
*/
	unsigned int sleepTime = 1000;
	while ((sleepTime = sleep(sleepTime)) != 0);

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
		"    -s <interval>    enable statistics in the specified interval in milliseconds\n" \
		"    -u <file>        output statistics in specified file (default: stats.log)\n"
	);
}


static int setup_signal(int signal, void (*handler)(int))
{
	struct sigaction sig;

	sig.sa_handler=handler;
	sig.sa_flags=SA_RESTART;
	sigemptyset(&sig.sa_mask);
	return(sigaction(signal, &sig, NULL));
}

static void sig_USR1_handler(int x)
{
	int errno_save = errno;

	printf("SIGUSR called\n");
	manager.parseConfig("configs/reconf2.xml");

	errno = errno_save;
}

/* just shallow right now */
static void sig_INT_handler(int x)
{
	static bool shutdownInitiated = false;

	if (shutdownInitiated) {
		msg(MSG_DIALOG, "second signal received, shutting down the hard way!");
		exit(2);
	}

	shutdownInitiated = true;
	msg(MSG_DIALOG, "got signal %d - exiting", x);

	manager.shutdown();

	// shut down all semaphores, so that shutdown will not be stalled due to some blocking stuff
	TimeoutSemaphore::shutdown();

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
