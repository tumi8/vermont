/*
 * Vermont 
 * Copyright (C) 2009 Vermont Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

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
#include "common/TimeoutSemaphore.h"
#include "common/msg.h"
#include "common/VermontControl.h"

#include "modules/ConfigManager.h"

using namespace std;

static void usage();
static void sig_INT_handler(int x);
static void sig_USR1_handler(int x);

static int setup_signal(int signal, void (*handler)(int));

ConfigManager manager;

// this is outside main to silent valgrind about the allocation "stats.log" which seems got never free'd
string statFile = "stats.log";

int main(int ac, char **dc)
{

	int c, debug_level=MSG_ERROR;
	char *config_file=NULL;

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
	
	if (sem_init(&mainSemaphore, 0, 0) == -1) {
		THROWEXCEPTION("failed to setup semaphore");
	}

	/* setup verboseness */
	msg(MSG_DIALOG, "message debug level is %d", debug_level);
	msg_setlevel(debug_level);

	setup_signal(SIGINT, sig_INT_handler);
	setup_signal(SIGUSR1, sig_USR1_handler);

	manager.parseConfig(string(config_file));

	sigset_t sigmask;
	sigemptyset(&sigmask);
	
	msg(MSG_DIALOG, "vermont is up and running");

	while (run_program) {		
		// sleep until we get a signal
		int s;
		while ((s = sem_wait(&mainSemaphore)) == -1 && errno == EINTR) {} // restart when interrupted by handler
		if (s == -1) THROWEXCEPTION("failed to execute sem_wait");

		if (reload_config) {
			msg(MSG_INFO, "reconfiguring vermont");
			manager.parseConfig(string(config_file));
			reload_config = false;
		}
	}
	manager.shutdown();
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

/* WARNING: don't use any of the msg/DPRINTF/etc functions in the SIGNAL handlers;
 *          they use an internal mutex to lock the display output so you get a deadlock
 */
static void sig_USR1_handler(int x)
{
	int errno_save = errno;

	DPRINTF("SIGUSR called");
	reload_config = true;
	
	errno = errno_save;
	wakeupMainThread();
}

/* just shallow right now */
static void sig_INT_handler(int x)
{
	int errno_save = errno;
	
	static bool shutdownInitiated = false;

	if (shutdownInitiated) {
		printf("second signal received, shutting down the hard way!");
		exit(2);
	}
	
	shutdownInitiated = true;

	msg(MSG_FATAL, "got signal %d - exiting", x);

	initiateShutdown();
	errno = errno_save;
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
