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

#include "PCAPExporterMem.h"

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

PCAPExporterMem::PCAPExporterMem(const std::string& logfile)
	: PCAPExporterPipe(logfile), shmfd(0), shm_list(NULL), currentListEntry(0), QUEUEENTRIES(8096*4)
{
}

PCAPExporterMem::~PCAPExporterMem()
{
}

/**
 * Starts the command given in 'cmd'
 * STDOUT and STDERR of 'cmd' may be redirected into a file, see
 * module_configuration.txt for details
 * */
int PCAPExporterMem::execCmd(std::string& cmd)
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
void PCAPExporterMem::performStart()
{
	time(&last_check);
	if(last_check == (time_t) -1)
		THROWEXCEPTION("time() failed");

	registerSignalHandlers();

	msg(MSG_INFO, "Started PCAPExporterMem with the following parameters:");
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
	msg(MSG_INFO, "  - PCAP_MAX_CAPTURE_LENGTH = %u seconds" , PCAP_MAX_CAPTURE_LENGTH);

	startProcess();
}

void PCAPExporterMem::performShutdown()
{
	std::string lockfile = "/tmp/" + boost::lexical_cast<std::string>(fifoReaderPid);
	std::string shutdown = "/tmp/shutdown_" + boost::lexical_cast<std::string>(fifoReaderPid);
	FILE *f = fopen(shutdown.c_str(), "w");
	fclose(f);
	while(bfs::exists(lockfile)){
	}	; // wait for snort to exit

	unregisterSignalHandlers();
	stopProcess();
}

void PCAPExporterMem::startProcess()
{
	int size = sizeof(SHMEntry)*QUEUEENTRIES;
	fifoReaderPid = execCmd(fifoReaderCmd);
	std::string name = "/" +  boost::lexical_cast<std::string>(fifoReaderPid);
	if ((shmfd = shm_open(name.c_str(), O_CREAT | O_EXCL | O_RDWR, 00666)) == -1){
		int err = errno;
		if(fifoReaderPid) kill_all(fifoReaderPid);
		THROWEXCEPTION("shm_open failed: %s", strerror(err));
	}

	if(ftruncate(shmfd, size)){
		int err = errno;
		if(fifoReaderPid) kill_all(fifoReaderPid);
		THROWEXCEPTION("ftruncate failed: %s", strerror(err));
	}
	shm_list = (SHMEntry *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
    if (shm_list == NULL) {
        perror("In mmap()");
        exit(1);
    }
	memset((void*)shm_list, 0, size);
	std::string lockfile = "/tmp/" + boost::lexical_cast<std::string>(fifoReaderPid);
	FILE *f = fopen(lockfile.c_str(), "w");
	fclose(f);


	msg(MSG_INFO, "Started process with fifoReaderCmd \'%s\' and fifoReaderPid = %d",
						fifoReaderCmd.c_str(), fifoReaderPid);
}

void PCAPExporterMem::stopProcess()
{
	std::string name = "/" + boost::lexical_cast<std::string>(fifoReaderPid);
	if (fifoReaderPid != 0 ) {
		kill_all(fifoReaderPid);
	}

	removeSHM(fifoReaderPid, shmfd);

	msg(MSG_INFO, "Stopped process with fifoReaderCmd \'%s\' and fifoReaderPid = %d",
					fifoReaderCmd.c_str(), fifoReaderPid);
}

/**
 * This method closes shmfd and deletes the shared memory segment
 * It is important to invoke this method whenever Vermont is shut down,
 * because shm segments are not automatically destroyed upon process 
 * termination!
 * TODO
 * */
int PCAPExporterMem::removeSHM(int pid, int des){
	int ret = 0;
	std::string name = "/" + boost::lexical_cast<std::string>(pid);
	ret += close(des);
	//ret += shm_unlink(name.c_str()); //snort must unlink the segment
	return ret;
}

/**
* Writes a packet into the pipe
*/
void PCAPExporterMem::receive(Packet* packet)
{
	DPRINTFL(MSG_VDEBUG, "PCAPExporterMem::receive() called");
	if (onRestart){
		 DPRINTF("Dropping incoming packet, as attached process is not ready");
		 DPRINTFL(MSG_VDEBUG, "PCAPExporterMem::receive() ended");
		 return;
	}
	if (fifoReaderPid == 0){
		 msg(MSG_VDEBUG, "fifoReaderPid = 0...this might happen during reconfiguration");
		 DPRINTFL(MSG_VDEBUG, "PCAPExporterMem::receive() ended");
		 return;
	}
	if (restartInterval) {
		if (nextRestart.tv_sec==0) {
			DPRINTFL(MSG_VDEBUG, "PCAPExporterMem::receive(): updating nextRestart");
			nextRestart = packet->timestamp;
			nextRestart.tv_sec += restartInterval;
		} else if (compareTime(nextRestart, packet->timestamp)<0) {
			DPRINTFL(MSG_VDEBUG, "PCAPExporterMem::receive(): restarting process");

			// we need to unregister our signal handlers, as we get race conditions with the signal handler for restarting the process
			unregisterSignalHandlers();
			stopProcess();
			startProcess();
			registerSignalHandlers();
			DPRINTFL(MSG_VDEBUG, "PCAPExporterMem::receive(): updating nextRestart");
			nextRestart.tv_sec += ((packet->timestamp.tv_sec-nextRestart.tv_sec)/restartInterval+1)*restartInterval;
		}
	}

	writeIntoMemory(packet);

	statBytesForwarded += packet->data_length;
	statPktsForwarded++;
	DPRINTFL(MSG_VDEBUG, "PCAPExporterMem::receive() ended");
}

void PCAPExporterMem::writeIntoMemory(Packet *packet){
START:
	while(! __sync_bool_compare_and_swap(&(shm_list[currentListEntry].locked), 0, 1)) {
		//wait until we got a lock on the entry
	}
	if(shm_list[currentListEntry].new_data == 1){
		shm_list[currentListEntry].locked = 0;
		goto START;
	}

	shm_list[currentListEntry].packetHeader.ts = packet->timestamp;
	shm_list[currentListEntry].packetHeader.caplen = packet->data_length;
	shm_list[currentListEntry].packetHeader.pktlen = packet->pcapPacketLength;
	shm_list[currentListEntry].packetHeader.device_index = 0;
	shm_list[currentListEntry].packetHeader.flags = 0;
	memcpy(shm_list[currentListEntry].data, packet->data, packet->data_length);

	shm_list[currentListEntry].new_data = 1;
	shm_list[currentListEntry].locked = 0;
	currentListEntry++;
	currentListEntry %= QUEUEENTRIES;
	packet->removeReference();
}


/**
 * Handles SIGCHLD and tries to restart the external process.
 * If this fails too often within a short time period, we assume that
 * something went terribly wrong and throw an exception accordingly.
 */
void PCAPExporterMem::handleSigChld(int sig)
{
	int pid = fifoReaderPid;
	int des = shmfd;
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
		removeSHM(pid, des);
		startProcess();
	}

	time(&last_check);
	if(last_check == (time_t) -1)
		THROWEXCEPTION("time() failed");
	onRestart = false;
}

