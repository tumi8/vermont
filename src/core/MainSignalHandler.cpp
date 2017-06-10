/*
 * Vermont
 * Copyright (C) 2010 Vermont Project
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

#include "MainSignalHandler.h"

MainSignalHandler::MainSignalHandler()
{
	SignalHandler::getInstance().registerSignalHandler(SIGINT, this);
	SignalHandler::getInstance().registerSignalHandler(SIGTERM, this);
	SignalHandler::getInstance().registerSignalHandler(SIGUSR1, this);
	SignalHandler::getInstance().registerSignalHandler(SIGUSR2, this);
	SignalHandler::getInstance().registerSignalHandler(SIGHUP, this);
}


/* WARNING: don't use any of the msg/DPRINTF/etc functions in the SIGNAL handlers;
 *          they use an internal mutex to lock the display output so you get a deadlock
 */

void MainSignalHandler::handleSigUsr1(int x)
{
	int errno_save = errno;

	int log_mask = msg_getlevel();

	if (!(log_mask & LOG_MASK(LOG_DEBUG))) {
		if (log_mask == 0) {
			log_mask = LOG_MASK(LOG_EMERG);
		} else {
			log_mask |= log_mask << 1;
		}
		msg_setlevel(log_mask);
	}

	errno = errno_save;
}

void MainSignalHandler::handleSigUsr2(int x)
{
	int errno_save = errno;
	int log_mask = msg_getlevel();

	if (log_mask != 0) {
		log_mask &= log_mask >> 1;
		msg_setlevel(log_mask);
	}

	errno = errno_save;
}

void MainSignalHandler::handleSigHup(int x)
{
	int errno_save = errno;

	reload_config = true;

	errno = errno_save;
	wakeupMainThread();
}

/* just shallow right now */
void MainSignalHandler::handleSigInt(int x)
{
	int errno_save = errno;

	static bool shutdownInitiated = false;

	if (shutdownInitiated) {
		printf("second signal received, shutting down the hard way!");
		exit(2);
	}

	shutdownInitiated = true;

	initiateShutdown();
	errno = errno_save;
}

/* just shallow right now */
void MainSignalHandler::handleSigTerm(int x)
{
	int errno_save = errno;

	static bool shutdownInitiated = false;

	if (shutdownInitiated) {
		printf("second signal received, shutting down the hard way!");
		exit(2);
	}

	shutdownInitiated = true;

	initiateShutdown();
	errno = errno_save;
}
