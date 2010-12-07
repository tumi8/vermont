/*
 * Vermont PCAP Exporter
 * Copyright (C) 2010 Vermont Project
 * Copyright (C) 2010 Tobias Limmer <limmer@cs.fau.de>
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

#include "PCAPExporterProcessBase.h"

#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>
#include <fstream>
#include <vector>
#include <cstring>
#include <boost/filesystem/operations.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <sys/wait.h>
#include <common/SignalHandler.h>
#include <boost/tokenizer.hpp>


namespace bfs = boost::filesystem;


PCAPExporterProcessBase::PCAPExporterProcessBase(const std::string& logfile)
	: logFileName(logfile),
	  fifoReaderCmd(""),
	  workingPath(""),
	  onRestart(false),
	  appenddate(false),
	  restartOnSignal(false),
	  fifoReaderPid(0),
	  sigKillTimeout(1),
	  counter(0),
	  last_check(0),
	  statPktsForwarded(0),
	  statBytesForwarded(0),
	  statPktsDropped(0),
	  statBytesDropped(0),
	  restartInterval(0)
{
	bzero(&nextRestart, sizeof(nextRestart));
}

PCAPExporterProcessBase::~PCAPExporterProcessBase()
{
}

void PCAPExporterProcessBase::setPipeReaderCmd(const std::string& cmd)
{
	fifoReaderCmd = cmd;
}

void PCAPExporterProcessBase::setWorkingPath(const std::string& path)
{
	workingPath = path;
}

void PCAPExporterProcessBase::setAppendDate(bool value)
{
	appenddate = value;
}

void PCAPExporterProcessBase::setRestartInterval(uint32_t ri)
{
	restartInterval = ri;
}

void PCAPExporterProcessBase::setSigKillTimeout(int s)
{
	if (s < 0) s *= -1;
	sigKillTimeout = s;
}

void PCAPExporterProcessBase::setRestartOnSignal(bool b){
	restartOnSignal = b;
}

void PCAPExporterProcessBase::registerSignalHandlers()
{
	SignalHandler::getInstance().registerSignalHandler(SIGCHLD, this);
	SignalHandler::getInstance().registerSignalHandler(SIGPIPE, this);
	if (restartOnSignal)
		SignalHandler::getInstance().registerSignalHandler(SIGUSR2, this);
}

void PCAPExporterProcessBase::unregisterSignalHandlers()
{
	SignalHandler::getInstance().unregisterSignalHandler(SIGCHLD, this);
	SignalHandler::getInstance().unregisterSignalHandler(SIGPIPE, this);
	if (restartOnSignal)
		SignalHandler::getInstance().unregisterSignalHandler(SIGUSR2, this);
}

void PCAPExporterProcessBase::handleSigPipe(int sig)
{
	handleSigChld(SIGCHLD);
}


/**
 * Handles SIGCHLD and tries to restart the external process.
 * If this fails too often within a short time period, we assume that
 * something went terribly wrong and throw an exception accordingly.
 */
void PCAPExporterProcessBase::handleSigChld(int sig)
{
	int pid = fifoReaderPid;
	if (onRestart || isRunning(pid)) return;
	onRestart = true;
	counter++;
	time_t tmp;
	time(&tmp);
	if(tmp== (time_t) -1)
		THROWEXCEPTION("time() failed");

	// reset the counter if the last restart was more than 5 seconds ago
	if((tmp -last_check) > 5) counter = 0;

	if(counter > 5 && (tmp - last_check ) < 5)
		THROWEXCEPTION("Too many restarts in a short time period. Maybe your commandline is erroneous");

	if(!isRunning(pid)){
		//waitpid(fifoReaderPid, NULL, 0);
		msg(MSG_ERROR, "Process of fifoReaderCmd \'%s\' with fifoReaderPid %d is not running!",
				fifoReaderCmd.c_str(), pid);
		startProcess();
	}

	time(&last_check);
	if(last_check == (time_t) -1)
		THROWEXCEPTION("time() failed");
	onRestart = false;
}

/**
 * Catches SIGUSR2 and restarts the external process
 * */
void PCAPExporterProcessBase::handleSigUsr2(int sig)
{
	if (! restartOnSignal) return;
	int pid = fifoReaderPid;
	fifoReaderPid = 0;
	kill_all(pid);
	handleSigChld(sig);
}


/**
 * Kills a single process by sending SIGTERM to 'pid'.
 * Waits an user-defined interval before
 * eventually sending SIGKILL to the process if it's still running
 */
void PCAPExporterProcessBase::kill_pid(int pid)
{
	int i = sigKillTimeout;
	std::string path = "/proc/" + boost::lexical_cast<std::string>(pid);
	/*msg(MSG_DEBUG, "Sending SIGTERM to pid %u", pid);
	if (kill(pid, SIGTERM)) {
		msg(MSG_ERROR, "Failed to call kill(%u, SIGTERM), error code %u (%s)", pid, errno, strerror(errno));
	}*/
	kill(pid, SIGTERM);
	while(i--){
		msg(MSG_INFO, "waiting for pid %d, but no longer than %d seconds...", pid, i+1);
		if (!isRunning(pid)) return;
		sleep(1);
	}
	msg(MSG_DEBUG, "Sending SIGKILL to pid %u", pid);
	if (kill(pid, SIGKILL)) {
		msg(MSG_ERROR, "Failed to call kill(%u, SIGKILL), error code %u (%s)", pid, errno, strerror(errno));
	}
	waitpid(pid, NULL, 0);
}

/**
 * Checks if the process is still running
 * */
bool PCAPExporterProcessBase::isRunning(int pid)
{
	int status;
	if(pid == 0) return false;
	int result = waitpid(pid, &status, WNOHANG);
	if(result == 0) return true;
	return false;
}


/**
 * Kills a process and all of its children
 * */
void PCAPExporterProcessBase::kill_all(int ppid)
{
	bfs::path path("/proc/");
	if (!bfs::exists(path) || !bfs::is_directory(path)){
		THROWEXCEPTION("Directory /proc not found...");
	}

	std::string line;
	std::vector<int> my_pids;
	std::vector<int> my_ppids;
	my_ppids.push_back(ppid);
	bfs::directory_iterator dir_iterator = bfs::directory_iterator(path);
	bfs::directory_iterator end_itr;
	while (dir_iterator != end_itr) {
		if (bfs::is_directory(dir_iterator->status())  &&
				this->checkint(dir_iterator->leaf().c_str()))
		{
			std::string filename = dir_iterator->path().file_string() + "/stat";
			std::ifstream myfile(filename.c_str());
			if (myfile.is_open()) {
				getline (myfile,line);
				char *token = strtok (const_cast<char*>(line.c_str())," ");
				int count = 0;
				while (token != NULL) {
					if (count++ == 3) { // field of parent pid
						for (std::vector<int>::iterator it = my_ppids.begin(); it != my_ppids.end(); it++) {
							if (atoi(token) == *it) {
								msg(MSG_DEBUG, "Pid %s is a child of %d", dir_iterator->leaf().c_str(), *it );
								my_pids.push_back(boost::lexical_cast<int>(dir_iterator->leaf()));
								my_ppids.push_back(boost::lexical_cast<int>(dir_iterator->leaf()));
								break;
							}
						}
					}
					token = strtok (NULL, " ");
				}
				myfile.close();
			}
		}
		dir_iterator++;
	}
	my_pids.push_back(ppid);
	for (std::vector<int>::iterator it = my_pids.begin(); it != my_pids.end(); it++) {
		kill_pid(*it);
	}
}


/**
 * statistics function called by StatisticsManager
 */
std::string PCAPExporterProcessBase::getStatisticsXML(double interval)
{
	ostringstream oss;
	oss << "<forwarded type=\"packets\">" << statPktsForwarded << "</forwarded>";
	oss << "<forwarded type=\"bytes\">" << statBytesForwarded << "</forwarded>";
	oss << "<dropped type=\"packets\">" << statPktsDropped << "</dropped>";
	oss << "<dropped type=\"bytes\">" << statBytesDropped << "</dropped>";
	if (isRunning(fifoReaderPid)) {
		oss << "<processInfo pid=\"" << fifoReaderPid << "\">";
		try {
			ThreadCPUInterface::JiffyTime jt = ThreadCPUInterface::getProcessJiffies(fifoReaderPid);
			oss << "<totalJiffies type=\"system\">" << jt.sysJiffies << "</totalJiffies>";
			oss << "<totalJiffies type=\"user\">" << jt.userJiffies << "</totalJiffies></processInfo>";
		}
		catch (std::runtime_error& re) {
			// do not fail miserably when statistics were not retrieved correctly ...
		}
	}
	return oss.str();
}

bool PCAPExporterProcessBase::getProcessStatistics(uint32_t& sysjiffies, uint32_t& userjiffies)
{
	if (!isRunning(fifoReaderPid)) return false;

	try {
		ThreadCPUInterface::JiffyTime jt = ThreadCPUInterface::getProcessJiffies(fifoReaderPid);
		sysjiffies = jt.sysJiffies;
		userjiffies = jt.userJiffies;
	}
	catch (std::runtime_error& re) {
		// do not fail miserably when statistics were not retrieved correctly ...
		return false;
	}

	return true;
}


void PCAPExporterProcessBase::getPacketStats(uint64_t& droppedpkts, uint64_t& forwpkts)
{
	droppedpkts = statPktsDropped;
	forwpkts = statPktsForwarded;
}


void PCAPExporterProcessBase::getOctetStats(uint64_t& droppedocts, uint64_t& forwocts)
{
	droppedocts = statBytesDropped;
	forwocts = statBytesForwarded;
}


bool PCAPExporterProcessBase::checkint(const char *my_string)
{
	size_t stringlength = strlen(my_string);
	for (size_t j=0; j<stringlength; j++)
		if((int)my_string[j] < 0x30 || (int)my_string[j] > 0x39)
			return false;

	return true;
}

/**
 * Splits a string into single tokens using whitespaces as delimiter
 */
void PCAPExporterProcessBase::parseCommandLine(std::string &cmd, std::vector<std::string> &tokens)
{
	using namespace boost;
	if (cmd.length() > 1023) {
		THROWEXCEPTION("Command too long");
	}
	//treat substrings containing whitespaces enclosed in quotation marks as single argument
	escaped_list_separator<char> sep('\\', ' ', '\"');
	tokenizer<escaped_list_separator<char> > tok(cmd, sep);
	for(tokenizer<escaped_list_separator<char> >::iterator it=tok.begin();
		it!=tok.end();
		++it)
	{
		tokens.push_back(*it);
	}
}


void PCAPExporterProcessBase::redirectLogfile()
{
	if (logFileName != "") {
		std::string logfile = logFileName;
		if(appenddate){
			time_t rawtime;
			struct tm * timeinfo;
			char buffer [20];
			time (&rawtime);
			timeinfo = localtime (&rawtime);
			if (strftime(buffer, 20, "%Y-%m-%d_%H:%M:%S", timeinfo) == 0) {
				if (write(child_parent_pipe[1], &errno, sizeof(int)) != sizeof(int))
					THROWEXCEPTION("strftime & write failed");
				exit(1);
			}
			logfile = logFileName + "_" + buffer + ".log";
		}

		if (freopen (logfile.c_str(),"a",stdout) == NULL) {
			if (write(child_parent_pipe[1], &errno, sizeof(int)) != sizeof(int))
				THROWEXCEPTION("freopen & write failed");
			exit(1);
		}
		if (dup2(STDOUT_FILENO, STDERR_FILENO) == -1) {
			if (write(child_parent_pipe[1], &errno, sizeof(int)) != sizeof(int))
				THROWEXCEPTION("dup2 & write failed");
			exit(1);
		}
	}
}
