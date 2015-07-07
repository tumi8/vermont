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
#include <getopt.h>
#include <pwd.h>
#include <grp.h>
#include <syslog.h>

/* own systems */
#include "common/msg.h"
#include "common/VermontControl.h"
#include "common/defs.h"
#include "core/MainSignalHandler.h"

#include "modules/ConfigManager.h"

using namespace std;

struct parameters {
	const char *config_file;
	const char *pid_file;
	uid_t uid;
	gid_t gid;
	int log_level;
	bool daemon_mode;
};


static void
record_pid (const char *pidfile)
{
	FILE *f = fopen(pidfile, "w");
	if (!f) {
		fprintf(stderr, "Could not open pid-file %s for writing", pidfile);
		exit(EXIT_FAILURE);
	} else {
		fprintf(f, "%d\n", getpid());
		fclose(f);
	}
}

static void
set_groups (uid_t uid, gid_t gid)
{
	int groups_count = 0;
	gid_t *groups = NULL;

	struct passwd *pw = getpwuid(uid);
	if (!pw) {
		fprintf(stderr, "could not getpwuid: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	/*
	 * With groups NULL and groups_count 0, getgrouplist will write the actual
	 * number of groups into groups_count.
	 */
	getgrouplist(pw->pw_name, gid, groups, &groups_count);

	/*
	 * List must be on the stack.
	 */
	groups = (gid_t *)alloca(groups_count * sizeof(gid_t));

	if (getgrouplist(pw->pw_name, gid, groups, &groups_count) <= 0) {
		fprintf(stderr, "could not getgrouplist: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (setgroups(groups_count, groups) < 0) {
		fprintf(stderr, "could not setgroups: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
}

static void
daemonise (const char *pid_file, uid_t uid, gid_t gid)
{
	/*
	 * 1 to stay in the current working dir, 0 to redirect stdin/out/err to
	 * dev/null
	 */
	if (daemon(1, 0) < 0) {
		fprintf(stderr, "daemon failed\n");
		exit(EXIT_FAILURE);
	}

	if (pid_file) {
		record_pid(pid_file);
	}

	if (gid) {
		set_groups(uid, gid);
	}

	if (uid && setreuid(0, uid) < 0) {
		fprintf(stderr, "setreuid %u fail: %s", uid, strerror(errno));
		exit(EXIT_FAILURE);
	}
}

static void
usage (int status)
{
	printf("VERsatile MONitoring Tool - VERMONT\n"
			" MANDATORY OPTIONS:\n"
			" -f, --config-file FILE     Use configuration file\n"
			"\n OTHER OPTIONS:\n"
			" -h, --help                 Display this help and exit\n"
			" -d, --debug                Log verbosity: -d NOTICE, -dd INFO,\n"
			"                                -ddd DEBUG\n"
			" -b, --daemon               Run in daemon mode (implies -q)\n"
			" -p, --pid-file FILE        Set process id filename (use with -d)\n"
			" -u, --user USER            Change user to USER (use with -d)\n"
			" -g, --group GROUP          Change group to GROUP (use with -d)\n"
	);

	exit(status);
}

static int
parse_args (int argc, char **argv, struct parameters *params)
{
	int opt, ret, option_index;
	struct passwd *pw;
	struct group *gr;

	static const struct option long_opts[] = {
			{ "help",         no_argument,       NULL, 'h' },
			{ "config-file",  required_argument, NULL, 'f' },
			{ "daemon",       no_argument,       NULL, 'b' },
			{ "pid-file",     required_argument, NULL, 'p' },
			{ "user",         required_argument, NULL, 'u' },
			{ "group",        required_argument, NULL, 'g' },
			{ "debug",        no_argument,       NULL, 'd' },
			{ NULL, 0, NULL, 0}
	};

	while ((opt = getopt_long(argc, argv, "hbp:u:g:df:", long_opts,
			&option_index)) != EOF) {
		switch (opt) {
		case 'b':
			params->daemon_mode = true;
			break;

		case 'f':
			params->config_file = optarg;
			break;

		case 'p':
			params->pid_file = optarg;
			break;

		case 'u':
			// Drop privileges and run as this user after initialisation.
			pw = getpwnam(optarg);
			if (!pw) {
				fprintf(stderr, "unknown user '%s'\n", optarg);
				exit(EXIT_FAILURE);
			}
			params->uid = pw->pw_uid;
			break;

		case 'g':
			// drop privileges and run as this group after initialisation.
			gr = getgrnam(optarg);
			if (!gr) {
				fprintf(stderr, "unknown group '%s'\n", optarg);
				exit(EXIT_FAILURE);
			}
			params->gid = gr->gr_gid;
			break;

		case 'h':
			usage(EXIT_SUCCESS);
			break;

		case 'd':
			params->log_level++;
			break;

		default:
			usage(EXIT_FAILURE);
			break;
		}
	}

	/*
	 * reset getopt library
	 */
	ret = optind - 1;
	optind = 0;

	return ret;
}

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
	struct parameters parameters;

	memset(&parameters, 0, sizeof(struct parameters));
	parameters.log_level = MSG_ERROR;

	/* parse command line */
	if (parse_args (ac, dc, &parameters) < 0) {
		exit(EXIT_FAILURE);
	}

	if (parameters.config_file == NULL) {
		msg(MSG_FATAL, "no config file given, but mandatory");
		usage(EXIT_FAILURE);
	}

	if (parameters.daemon_mode) {
		daemonise(parameters.pid_file, parameters.uid, parameters.gid);
	}

	msg_init();
	/**< Wrapper for the main thread's signal handlers*/
	MainSignalHandler main_signal_handler;

#ifdef __APPLE__
	if (semaphore_create(mach_task_self(), &mainSemaphore, SYNC_POLICY_FIFO, 0) != KERN_SUCCESS) {
#else
	if (sem_init(&mainSemaphore, 0, 0) == -1) {
#endif
		THROWEXCEPTION("failed to setup semaphore");
	}

	/* setup verboseness */
	msg(MSG_DIALOG, "message debug level is %d", parameters.log_level);
	msg_setlevel(parameters.log_level);


	manager.parseConfig(string(parameters.config_file));

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
			manager.parseConfig(string(parameters.config_file));
			reload_config = false;
		}
	}
	msg(MSG_FATAL, "got signal - exiting");
	manager.shutdown();
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
