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

#include "PCAPExporterQueue.h"

#include "modules/packet/Packet.h"
#include "common/Time.h"

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

namespace bfs = boost::filesystem;

PCAPExporterQueue::PCAPExporterQueue(const std::string& logfile, uint32_t maxqueuemsgs)
	: PCAPExporterPipe(logfile), queuedes(0), maxQueueMsgs(maxqueuemsgs)
{
}

PCAPExporterQueue::~PCAPExporterQueue()
{
}

/**
 * Starts the command given in 'cmd'
 * STDOUT and STDERR of 'cmd' may be redirected into a file, see
 * module_configuration.txt for details
 * */
int PCAPExporterQueue::execCmd(std::string& cmd)
{
	int child_parent_pipe[2];
	char *command[64];
	char tmp[1024];
	if (strlen(cmd.c_str()) > 1023) {
		THROWEXCEPTION("Command too long");
	}
	strcpy(tmp, cmd.c_str());
	command[0] = strtok(tmp, " \t");
	int i = 0;
	//TODO:  handle arguments containg whitespaces
	while (command[i]) {
		i++;
		if (i == 63) {
			THROWEXCEPTION("Argument list too long");
		}
		command[i] = strtok(NULL, " \t");
	}
	command[++i] = (char*)0;
	if (pipe(child_parent_pipe)) {
		THROWEXCEPTION("pipe(child_parent_pipe) failed with error code %u (%s)", errno, strerror(errno));
	}

	/* Create a pipe, which allows communication between the child and the parent.
	 * Writing an int value (e.g. errno) into child_parent_pipe[1]
	sleep(2);
	 * will cause an exception in the parent process.
	 * Throwing exceptions in the child *will not* terminate Vermont!
	 */
	for (int i=0; i<2; i++) {
		int oldflags = fcntl (child_parent_pipe[i], F_GETFD, 0);
		if(-1 == oldflags)
			THROWEXCEPTION("fctnl: %s", strerror(errno));
		oldflags |= FD_CLOEXEC;
		if (-1 == fcntl (child_parent_pipe[i], F_SETFD, oldflags))
			THROWEXCEPTION("fctnl: %s", strerror(errno));
	}

	int pid = fork();
	if (pid == 0) {
		// child process
		close(child_parent_pipe[0]); // close read-end
		if (workingPath != "") {
			int res = chdir(workingPath.c_str());
			if (res != 0) {
				THROWEXCEPTION("failed to change to working path '%s'", workingPath.c_str());
				exit(1);
			}
		}
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

		if (execvp(command[0], command)<0) {
			if (write(child_parent_pipe[1], &errno, sizeof(int)) != sizeof(int))
				THROWEXCEPTION("exec failed"); //throw the exception only if we couldn't
												//tell the parent that sth. went wrong...
			exit(1);
		}
	} else if (pid > 0) {
		close(child_parent_pipe[1]);
		int buf;
		if (read(child_parent_pipe[0], &buf, sizeof(int)) == sizeof(int)) { //The child actually wrote errno into the pipe
			THROWEXCEPTION("An error occurred in the child: %s", strerror(buf));
		}
		close(child_parent_pipe[0]);

		return pid;
	} else {
		THROWEXCEPTION("fork() failed");
	}
	return -1;
}

/**
 * Startup method for the module
 */
void PCAPExporterQueue::performStart()
{
	time(&last_check);
	if(last_check == (time_t) -1)
		THROWEXCEPTION("time() failed");

	registerSignalHandlers();

	msg(MSG_INFO, "Started PCAPExporterQueue with the following parameters:");
	if (fifoReaderCmd != ""){
		msg(MSG_INFO, "  - fifoReaderCmd = %s", fifoReaderCmd.c_str());
		msg(MSG_INFO, "  - fifoReaderPid = %d", fifoReaderPid);
	} else {
		THROWEXCEPTION("No fifoReaderCmd specified!");
	}
	if (logFileName != ""){
		msg(MSG_INFO, "  - logfileBaseName = %s", logFileName.c_str());
		msg(MSG_INFO, "  - appenddate = %s", appenddate ? "true" : "false");
	}
	else
		msg(MSG_ERROR, "No Logfile specified - dumping to stdout!");
	msg(MSG_INFO, "  - sigKillTimeout = %d" , sigKillTimeout);
	msg(MSG_INFO, "  - restartInterval = %u seconds" , restartInterval);
	msg(MSG_INFO, "  - maxQueueSize = %u entries" , maxQueueMsgs);

	startProcess();
}

void PCAPExporterQueue::performShutdown()
{
	unregisterSignalHandlers();
	stopProcess();
}

void PCAPExporterQueue::startProcess()
{
	struct mq_attr attr;
	struct rlimit limit;
	limit.rlim_cur = RLIM_INFINITY;
	limit.rlim_max = RLIM_INFINITY;

	attr.mq_flags = 0; //O_NONBLOCK;     
	attr.mq_maxmsg = maxQueueMsgs;
	attr.mq_msgsize = sizeof(QueueMessage);

	freeEntries = maxQueueMsgs;

	if(setrlimit(RLIMIT_MSGQUEUE, &limit) == -1)
		THROWEXCEPTION("setrlimit failed: %s", strerror(errno));

	fifoReaderPid = execCmd(fifoReaderCmd);
	std::string name = "/" +  boost::lexical_cast<std::string>(fifoReaderPid);
	if ((queuedes = mq_open(name.c_str(), O_CREAT|O_WRONLY, 00666, &attr)) == (mqd_t)-1){
		int err = errno;
		if(fifoReaderPid) kill_all(fifoReaderPid);
		THROWEXCEPTION("mq_open failed: %s", strerror(err));
	}

#if 0
	if((mqd_t)-1 == mq_setattr(queuedes, &attr, NULL)){
		THROWEXCEPTION("Couldn't set flag for message queue: %s", strerror(errno));
	}

#endif
	msg(MSG_INFO, "Started process with fifoReaderCmd \'%s\' and fifoReaderPid = %d",
						fifoReaderCmd.c_str(), fifoReaderPid);
}

void PCAPExporterQueue::stopProcess()
{
	std::string name = "/" + boost::lexical_cast<std::string>(fifoReaderPid);
	if (fifoReaderPid != 0 ) {
		kill_all(fifoReaderPid);
	}
	/*if((mqd_t)-1 == mq_close(queuedes)){
		THROWEXCEPTION("mq_close failed: %s", strerror(errno));
	}
	if((mqd_t)-1 == mq_unlink(name.c_str())){
		THROWEXCEPTION("mq_close failed: %s", strerror(errno));
	}*/
	closeQueue(fifoReaderPid, queuedes);

	msg(MSG_INFO, "Stopped process with fifoReaderCmd \'%s\' and fifoReaderPid = %d",
					fifoReaderCmd.c_str(), fifoReaderPid);
}

/**
 * This method closes and deletes the current queue.
 * It is important to invoke this method whenever Vermont is shut down,
 * because message queues are not automatically destroyed upon process 
 * termination!
 * TODO
 * */
int PCAPExporterQueue::closeQueue(int pid, mqd_t des){
	int ret = 0;
	std::string name = "/" + boost::lexical_cast<std::string>(pid);
	ret += (int)mq_close(des);
	ret += mq_unlink(name.c_str());
	return ret;
}

/**
* Writes a packet into the pipe
*/
void PCAPExporterQueue::receive(Packet* packet)
{
	DPRINTFL(MSG_VDEBUG, "PCAPExporterQueue::receive() called");
	if (onRestart){
		 DPRINTF("Dropping incoming packet, as attached process is not ready");
		 DPRINTFL(MSG_VDEBUG, "PCAPExporterQueue::receive() ended");
		 return;
	}
	if (fifoReaderPid == 0){
		 msg(MSG_VDEBUG, "fifoReaderPid = 0...this might happen during reconfiguration");
		 DPRINTFL(MSG_VDEBUG, "PCAPExporterQueue::receive() ended");
		 return;
	}
	if (restartInterval) {
		if (nextRestart.tv_sec==0) {
			DPRINTFL(MSG_VDEBUG, "PCAPExporterQueue::receive(): updating nextRestart");
			nextRestart = packet->timestamp;
			nextRestart.tv_sec += restartInterval;
		} else if (compareTime(nextRestart, packet->timestamp)<0) {
			DPRINTFL(MSG_VDEBUG, "PCAPExporterQueue::receive(): restarting process");

			// we need to unregister our signal handlers, as we get race conditions with the signal handler for restarting the process
			unregisterSignalHandlers();
			stopProcess();
			startProcess();
			registerSignalHandlers();
			DPRINTFL(MSG_VDEBUG, "PCAPExporterQueue::receive(): updating nextRestart");
			nextRestart.tv_sec += ((packet->timestamp.tv_sec-nextRestart.tv_sec)/restartInterval+1)*restartInterval;
		}
	}

	/*if (freeEntries<2) {
		struct mq_attr attr;
		if (mq_getattr(queuedes, &attr)!=0)
			THROWEXCEPTION("PCAPExporterQueue: failed to call mq_gettattr, error %u (%s)", errno, strerror(errno));
		freeEntries = attr.mq_curmsgs;
	}*/
	//if (freeEntries>=2) {
	if (dumpIntoQueue(packet)) {
		freeEntries -= 2;
		statBytesForwarded += packet->data_length;
		statPktsForwarded++;
	} else {
		statBytesDropped += packet->data_length;
		statPktsDropped++;
	}

	packet->removeReference();
	DPRINTFL(MSG_VDEBUG, "PCAPExporterQueue::receive() ended");
}

bool PCAPExporterQueue::dumpIntoQueue(Packet *packet)
{
	QueueMessage sendme;
	struct daq_pkthdr packetHeader;
	packetHeader.ts = packet->timestamp;
	packetHeader.caplen = packet->data_length;
	packetHeader.pktlen = packet->pcapPacketLength;
	packetHeader.device_index = 0;
	packetHeader.flags = 0;
	struct timespec timeout;
	timeout.tv_sec = 0;
	timeout.tv_nsec = 0;
	if ((mqd_t)-1 == mq_timedsend(queuedes, (char*)&packetHeader, sizeof(struct daq_pkthdr), 0, &timeout)) {
		if (errno==ETIMEDOUT) {
			return false;
		} else if(errno == EAGAIN){
			msg(MSG_INFO, "EAGAIN");
		} else {
			closeQueue(fifoReaderPid, queuedes);
			THROWEXCEPTION("Couldn't send message: %u (%s)", errno, strerror(errno));
		}
	}

	//memcpy(&(sendme.data), packet->data, sendme.packetHeader.caplen);
	//pcap_dump((unsigned char*)dumper, &packetHeader, packet->data);
	if ((mqd_t)-1 == mq_send(queuedes, (char*)(packet->data), sizeof(char)*packetHeader.caplen, 0)){
		if(errno == EAGAIN){
			msg(MSG_INFO, "EAGAIN");
		}else {
			closeQueue(fifoReaderPid, queuedes);
			THROWEXCEPTION("Couldn't send message: %", strerror(errno));
		}
	}
	return true;
}


/**
 * Handles SIGCHLD and tries to restart the external process.
 * If this fails too often within a short time period, we assume that
 * something went terribly wrong and throw an exception accordingly.
 */
void PCAPExporterQueue::handleSigChld(int sig)
{
	int pid = fifoReaderPid;
	mqd_t des = queuedes;
	if(onRestart || exitFlag || isRunning(pid)) return;
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
		closeQueue(pid, des);
		startProcess();
	}

	time(&last_check);
	if(last_check == (time_t) -1)
		THROWEXCEPTION("time() failed");
	onRestart = false;
}

std::string PCAPExporterQueue::getStatisticsXML(double interval)
{
	ostringstream oss;
	struct mq_attr attr;
	if (mq_getattr(queuedes, &attr)!=0)
		THROWEXCEPTION("PCAPExporterQueue: failed to call mq_gettattr, error %u (%s)", errno, strerror(errno));
	oss << "<freeQueueEntries>" << attr.mq_curmsgs << "</freeQueueEntries>";
	oss << PCAPExporterPipe::getStatisticsXML(interval);
	return oss.str();
}
