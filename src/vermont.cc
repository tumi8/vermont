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
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* own systems */
#include "common/msg.h"
#include "common/VermontControl.h"
#include "common/defs.h"
#include "core/MainSignalHandler.h"

#include "modules/ConfigManager.h"

using namespace std;

static void usage();


int main(int ac, char **dc)
{
	/* manager is defined at the scope of the main function in order to
	 * steer clear of headaches which are related to the order of
	 * initialisation of static objects.
	 * Let's look at an example to better understand this requirement: The
	 * constructor of ConfigMananger indirectly uses the logging facility.
	 * As a result, we have to make sure that all static objects related to
	 * the logging facility are initialized before the c'tor of
	 * ConfigManager makes use of them. By defining manager inside main(),
	 * we ensure that all static objects have been initialized by the time
	 * the c'tor of ConfigManager is run. The same goes for the
	 * destructors: The d'tor of ConfigManager must be executed *before*
	 * the d'tors of the logging facility because the former makes use of
	 * the latter. */
	ConfigManager manager;
	string statFile = "stats.log";

	int c, debug_level=MSG_ERROR;
	char *config_file=NULL;

	msg_init();
	/**< Wrapper for the main thread's signal handlers*/
	MainSignalHandler main_signal_handler;

	/* parse command line */
	while ((c=getopt(ac, dc, "hf:d")) != -1) {

		switch (c) {

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

#ifdef __APPLE__
	if (semaphore_create(mach_task_self(), &mainSemaphore, SYNC_POLICY_FIFO, 0) != KERN_SUCCESS) {
#else
	if (sem_init(&mainSemaphore, 0, 0) == -1) {
#endif
		THROWEXCEPTION("failed to setup semaphore");
	}

	/* setup verboseness */
	msg(MSG_DIALOG, "message debug level is %d", debug_level);
	msg_setlevel(debug_level);


	manager.parseConfig(string(config_file));

	sigset_t sigmask;
	sigemptyset(&sigmask);

	msg(MSG_DIALOG, "vermont is up and running");

	while (run_program) {
		// sleep until we get a signal
        bool b;
	while (((b=timeoutsem.wait(DELETER_PERIOD)) == true) && errno == EINTR) {}// restart when interrupted by handler

		if (b == false){
			manager.onTimeout2();
		}

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
		"    -d               increase debug level (specify multiple for higher debug levels)\n"
	);
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
