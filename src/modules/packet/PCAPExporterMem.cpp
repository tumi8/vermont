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
#include "osdep/linux/sysinfo.h"

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
	: PCAPExporterPipe(logfile), shmfd(0), queuefd(0), shm_list(NULL), queuevarspointer(0),
	queueentries(1024), packetcount(0), dropcount(0)
{
	if (PCAP_MAX_CAPTURE_LENGTH>1600)
		 THROWEXCEPTION("PCAPExporterMem: PCAP_MAX_CAPTURE_LENGTH must be <=1600 bytes, it is %u now", PCAP_MAX_CAPTURE_LENGTH);
}

PCAPExporterMem::~PCAPExporterMem()
{
}

void PCAPExporterMem::setQueueEntries(int q)
{
	queueentries = q;
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
	if (last_check == (time_t) -1)
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
	msg(MSG_INFO, "  - queueentries = %u " , queueentries);

	startProcess();
}

void PCAPExporterMem::performShutdown()
{
	unregisterSignalHandlers();
	stopProcess();
	msg(MSG_INFO, "PCAPExporterMem: sent %u packets, dropped %u packets", packetcount, dropcount);
}

void PCAPExporterMem::startProcess()
{
	int size = sizeof(SHMEntry)*(queueentries+1);
	fifoReaderPid = execCmd(fifoReaderCmd);
	std::string name = "/" +  boost::lexical_cast<std::string>(fifoReaderPid);
	shm_list = (SHMEntry *) getNewSharedMemory(&shmfd, size, name);
	createQueue(queueentries);
	memset((void*)shm_list, 0, size);

	msg(MSG_INFO, "PCAPExporterMem: started process with fifoReaderCmd \'%s\' and fifoReaderPid = %d",
						fifoReaderCmd.c_str(), fifoReaderPid);
}

void PCAPExporterMem::stopProcess()
{
	std::string name = "/" + boost::lexical_cast<std::string>(fifoReaderPid);
	if (fifoReaderPid != 0 ) {
		kill_all(fifoReaderPid);
	}

	removeSHM(fifoReaderPid);

	msg(MSG_INFO, "PCAPExporterMem: stopped process with fifoReaderCmd \'%s\' and fifoReaderPid = %d",
					fifoReaderCmd.c_str(), fifoReaderPid);
}

/**
 * This method closes shmfd and deletes the shared memory segment
 * It is important to invoke this method whenever Vermont is shut down,
 * because shm segments are not automatically destroyed upon process 
 * termination!
 * It's possible to delete the shm regions by deleting the
 * corresponding files in /dev/shm, however this should only
 * be necessary when Vermont wasn't shutdown correctly
 * */
int PCAPExporterMem::removeSHM(int pid){
	int ret = 0;
	std::string name = "/" + boost::lexical_cast<std::string>(pid);
	ret += close(shmfd);
	ret += close(queuefd);
	ret += shm_unlink(name.c_str());
	name += "_queuevars";
	ret += shm_unlink(name.c_str());
	return ret;
}

/**
* Receives a packet and tries to write the packet into the ringbuffer
*/
void PCAPExporterMem::receive(Packet* packet)
{
	DPRINTFL(MSG_VDEBUG, "PCAPExporterMem::receive() called");
	if (onRestart) {
		 DPRINTF("Dropping incoming packet, as attached process is not ready");
		 DPRINTFL(MSG_VDEBUG, "PCAPExporterMem::receive() ended");
		 return;
	}
	if (fifoReaderPid == 0) {
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

	if (writeIntoMemory(packet)) {
		msg(MSG_VDEBUG, "PCAPExporterMem::receive(): wrote packet %d at pos %u", ++packetcount, *nextWrite);
	} else {
		batchUpdate();
		nanosleep(&spinLockTimeoutProducer, NULL);
		msg(MSG_VDEBUG, "PCAPExporterMem::receive(): dropped packet %d ", ++dropcount);
	}
	statBytesForwarded += packet->data_length;
	statPktsForwarded++;
	packet->removeReference();
	DPRINTFL(MSG_VDEBUG, "PCAPExporterMem::receive() ended");
}


/* Tries to write a packet into the shared memory
 * If there's no slot available, false is returned and no
 * packet will be written.
 */
bool PCAPExporterMem::writeIntoMemory(Packet *packet)
{
	uint32_t afterNextWrite = next(*nextWrite);

	if (afterNextWrite == *localRead) {
		if (afterNextWrite == *glob_read) {
			return false;
		}
		*localRead = *glob_read;
	}

	shm_list[*nextWrite].packetHeader.ts = packet->timestamp;
	shm_list[*nextWrite].packetHeader.caplen = packet->data_length;
	shm_list[*nextWrite].packetHeader.pktlen = packet->pcapPacketLength;
	shm_list[*nextWrite].packetHeader.device_index = 0;
	shm_list[*nextWrite].packetHeader.flags = 0;
	memcpy(shm_list[*nextWrite].data, packet->data, packet->data_length);

	*nextWrite = afterNextWrite;
	(*wBatch)++;
	if (*wBatch >= *batchSize) {
		*glob_write = *nextWrite;
		*wBatch = 0;
	}

	return true;
}


/**
 * Handles SIGCHLD and tries to restart the external process.
 * If this fails too often within a short time period, we assume that
 * something went terribly wrong and throw an exception accordingly.
 */
void PCAPExporterMem::handleSigChld(int sig)
{
	int pid = fifoReaderPid;
	if(onRestart || exitFlag || isRunning(pid)) return;
	onRestart = true;
	counter++;
	time_t tmp;
	time(&tmp);
	if (tmp== (time_t) -1)
		THROWEXCEPTION("time() failed");

	// reset the counter if the last restart was more than 5 seconds ago
	if ((tmp -last_check) > 5) 
		counter = 0;

	if (counter > 5 && (tmp - last_check ) < 5)
		THROWEXCEPTION("Too many restarts in a short time period. Maybe your commandline is erroneous");

	if (!isRunning(pid)) {
		//waitpid(fifoReaderPid, NULL, 0);
		msg(MSG_ERROR, "PCAPExporterMem: Process of fifoReaderCmd \'%s\' with fifoReaderPid %d is not running!",
				fifoReaderCmd.c_str(), pid);
		removeSHM(pid);
		startProcess();
	}

	time(&last_check);
	if (last_check == (time_t) -1)
		THROWEXCEPTION("time() failed");
	onRestart = false;
}

/* Creates a new shared memory of size "size".
 * Returns a pointer to the newly allocated memory
 */
void *PCAPExporterMem::getNewSharedMemory(int *fd, int size, std::string name)
{
	if ((*fd = shm_open(name.c_str(), O_CREAT | O_EXCL | O_RDWR, 00666)) == -1) {
		int err = errno;
		if (fifoReaderPid) 
			kill_all(fifoReaderPid);
		THROWEXCEPTION("shm_open failed: %s", strerror(err));
	}
	if (ftruncate(*fd, size)) {
		int err = errno;
		if(fifoReaderPid) kill_all(fifoReaderPid);
		THROWEXCEPTION("ftruncate failed: %s", strerror(err));
	}
	void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, *fd, 0);
	if (ptr == NULL) {
		perror("In mmap()");
		exit(1);
	}
	return ptr;
}

/*	Sets up the configuratiuon variables for the queue and places them into a shared memory
 *	region which is accessible via "/dev/shm/fiforeaderpid_queuevars".
 */
void PCAPExporterMem::createQueue(int maxEntries)
{
	uint32_t clsize = getCachelineSize();
	std::string name = "/" +  boost::lexical_cast<std::string>(fifoReaderPid) + "_queuevars";

	if (clsize < 3 * sizeof(uint32_t))
		THROWEXCEPTION("Error: Systems cache-line size is too small");

	void *ptr = getNewSharedMemory(&queuefd, clsize*5, name); //get a shared memory segment of 5*cachelinesize bytes
	queuevarspointer = ptr;


	uint32_t t = (uint32_t)ptr;
	while(t % clsize) t++;
	
	void *tmp = (void*)t;

	/*shared control variables*/
	glob_read = (uint32_t*)tmp;
	glob_write = glob_read + 1;
	*glob_read = *glob_write = 0;

	/*consumer’s local variables*/
	localWrite = (uint32_t*)tmp + (clsize/sizeof(uint32_t*));
	nextRead = localWrite + 1;
	rBatch = nextRead + 1;
	*localWrite = *nextRead = *rBatch = 0;

	/*producer’s local variables*/
	localRead = (uint32_t*)tmp + 2*(clsize/sizeof(uint32_t*));
	nextWrite = localRead + 1;
	wBatch = nextWrite + 1;
	*localRead = *nextWrite = *wBatch = 0;

	/*constants*/
	max = (uint32_t*)tmp + 3*(clsize/sizeof(uint32_t*));
	batchSize = max + 1;
	*max = maxEntries+1;
	*batchSize = 10;

	spinLockTimeoutProducer.tv_sec = 0;
	spinLockTimeoutProducer.tv_nsec = 51;
}

