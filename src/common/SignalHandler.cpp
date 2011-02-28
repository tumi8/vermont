/*
 * Vermont PCAP Exporter
 * Copyright (C) 2010 Vermont Project
 * Copyright (C) 2010 Thomas Hauenstein <sithhaue@users.berlios.de>
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
#include "SignalHandler.h"

SignalHandler::SignalHandler()		
{
	pthread_mutex_init(&mutex,0);
}

SignalHandler& SignalHandler::getInstance()
{
	static SignalHandler instance;
	return instance;
}

int SignalHandler::registerSignalHandler(int sig, SignalInterface *t)
{
	std::map<int, std::list<SignalInterface*> >::iterator it;
	it = signalList.find(sig);
	
	pthread_mutex_lock(&mutex);
	if (it == signalList.end()) {
		if (setupSignal(sig))
			THROWEXCEPTION("Couldn't set up signal handler for signal %d\n", sig);
	}
	signalList[sig].push_back(t);
	pthread_mutex_unlock(&mutex);
	return 0;
}

int SignalHandler::unregisterSignalHandler(int sig, SignalInterface *t)
{
	std::map<int, std::list<SignalInterface*> >::iterator it;

	it = signalList.find(sig);
	/**< no signal registered */
	if (it == signalList.end()) {
		return 1;
	}
	
	pthread_mutex_lock(&mutex);

	signalList[sig].remove(t);
	if (signalList[sig].empty()) {
		 signal(sig, SIG_DFL);
	}

	pthread_mutex_unlock(&mutex);
	return 0;
}

void SignalHandler::handleSignal(int sig)
{
	std::map<int, std::list<SignalInterface*> >::iterator it;
	it = signalList.find(sig);
	if (it == signalList.end()) {
		return;
	}

	for(std::list<SignalInterface*>::iterator jt = it->second.begin(); jt != it->second.end(); jt++){
		/**Any signals added here must also be included 
		 * in src/common/SignalInterface.h
		 */
		switch (sig) {
			case SIGALRM:
				(*jt)->handleSigAlrm(sig);
				break;
			case SIGCHLD:
				(*jt)->handleSigChld(sig);
				break;
			case SIGHUP:
				(*jt)->handleSigHup(sig);
				break;
			case SIGINT:
				(*jt)->handleSigInt(sig);
				break;
			case SIGPIPE:
				(*jt)->handleSigPipe(sig);
				break;
			case SIGQUIT:
				(*jt)->handleSigQuit(sig);
				break;
			case SIGTERM:
				(*jt)->handleSigTerm(sig);
				break;
			case SIGUSR1:
				(*jt)->handleSigUsr1(sig);
				break;
			case SIGUSR2:
				(*jt)->handleSigUsr2(sig);
				break;
			default:
				msg(MSG_ERROR, "No signal handler available for signal with number %d", sig);
				break;
		}
	}
}
int SignalHandler::setupSignal(int signal)
{
	struct sigaction sig;

	sig.sa_handler=handleSignalWrapper;
	sig.sa_flags=SA_RESTART;
	sigemptyset(&sig.sa_mask);
	return(sigaction(signal, &sig, NULL));
}

void SignalHandler::handleSignalWrapper(int sig) 
{
	getInstance().handleSignal(sig);
}

