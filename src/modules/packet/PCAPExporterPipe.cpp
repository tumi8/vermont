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

#include "PCAPExporterPipe.h"

#include "modules/packet/Packet.h"
#include "common/Time.h"

#include <fcntl.h>
#include <cstdlib>
#include <sys/types.h>
#include <sys/stat.h>



PCAPExporterPipe::PCAPExporterPipe(const std::string& logfile)
	: PCAPExporterProcessBase(logfile)
{
}

PCAPExporterPipe::~PCAPExporterPipe()
{
}


/**
 * Starts the command given in 'cmd'
 * STDOUT and STDERR of 'cmd' may be redirected into a file, see
 * module_configuration.txt for details
 * */
int PCAPExporterPipe::execCmd(std::string& cmd)
{
	char *command[64]; 
	int i=0;
	std::vector<std::string> tokens;
	parseCommandLine(cmd, tokens);
	for (std::vector<std::string>::iterator it = tokens.begin(); it!=tokens.end(); it++){
		command[i++] = const_cast<char*>(it->c_str());
		if(i>62) THROWEXCEPTION("Argument list too long");
	}
	command[i] = (char*)NULL;
		
	if (pipe(child_parent_pipe)) {
		THROWEXCEPTION("pipe(child_parent_pipe) failed with error code %u (%s)", errno, strerror(errno));
	}

	/* Create a pipe, which allows communication between the child and the parent.
	 * Writing an int value (e.g. errno) into child_parent_pipe[1]
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
		close(fd[1]); // close write-end
		close(child_parent_pipe[0]); // close read-end
		if (dup2(fd[0], STDIN_FILENO) == -1) {
			if (write(child_parent_pipe[1], &errno, sizeof(int)) != sizeof(int))
				THROWEXCEPTION("dup(fd[0]) failed");
				exit(1);
		}
		if (workingPath != "") {
			int res = chdir(workingPath.c_str());
			if (res != 0) {
				THROWEXCEPTION("failed to change to working path '%s'", workingPath.c_str());
				exit(1);
			}
		}
		redirectLogfile();

		if (execvp(command[0], command)<0) {
			if (write(child_parent_pipe[1], &errno, sizeof(int)) != sizeof(int))
				THROWEXCEPTION("exec failed"); //throw the exception only if we couldn't
												//tell the parent that sth. went wrong...
			exit(1);
		}
	} else if (pid > 0) {
		close(child_parent_pipe[1]);
		close(fd[0]);
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
void PCAPExporterPipe::performStart()
{
	time(&last_check);
	if(last_check == (time_t) -1)
		THROWEXCEPTION("time() failed");

	registerSignalHandlers();

	msg(MSG_INFO, "Started PCAPExporterPipe with the following parameters:");
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

	startProcess();
}


// some magic values taken from the version of libpcap where we got the code
#define TCPDUMP_MAGIC		0xa1b2c3d4
#define PCAP_VERSION_MAJOR 	2
#define PCAP_VERSION_MINOR 	4

void PCAPExporterPipe::startProcess()
{
	if (pipe(fd)) {
		THROWEXCEPTION("pipe() command failed");
	}

	fifoReaderPid = execCmd(fifoReaderCmd);
	msg(MSG_INFO, "Started process with fifoReaderCmd \'%s\' and fifoReaderPid = %d",
						fifoReaderCmd.c_str(), fifoReaderPid);

	pcapFile = fd[1];
	if (fcntl(pcapFile, F_SETFL, fcntl(pcapFile, F_GETFL) | O_NONBLOCK)==-1)
		THROWEXCEPTION("PCAPExporterPipe: fcntl failed, error %d (%s)", errno, strerror(errno));
	struct pcap_file_header hdr;
	hdr.magic = TCPDUMP_MAGIC;
	hdr.version_major = PCAP_VERSION_MAJOR;
	hdr.version_minor = PCAP_VERSION_MINOR;
	hdr.thiszone = 0;
	hdr.snaplen = snaplen;
	hdr.sigfigs = 0;
	hdr.linktype = DLT_EN10MB;

    if (write(pcapFile, (char *)&hdr, sizeof(hdr)) != sizeof(hdr))
        THROWEXCEPTION("PCAPExporterPipe: failed to write: %u (%s)", errno, strerror(errno));
}

void PCAPExporterPipe::stopProcess()
{
	close(pcapFile);

	if (fifoReaderPid != 0 ) {
		kill_all(fifoReaderPid);
	}

	msg(MSG_INFO, "Stopped process with fifoReaderCmd \'%s\' and fifoReaderPid = %d",
						fifoReaderCmd.c_str(), fifoReaderPid);
}

/**
 * Writes a packet into the pipe
 */
void PCAPExporterPipe::receive(Packet* packet)
{
	DPRINTFL(MSG_VDEBUG, "PCAPExporterPipe::receive() called");
	if (onRestart){
		 DPRINTF("Dropping incoming packet, as attached process is not ready");
		 DPRINTFL(MSG_VDEBUG, "PCAPExporterPipe::receive() ended");
		 packet->removeReference();
		 return;
	}
	if (fifoReaderPid == 0){
		 msg(MSG_VDEBUG, "fifoReaderPid = 0...this might happen during reconfiguration");
		 DPRINTFL(MSG_VDEBUG, "PCAPExporterPipe::receive() ended");
		 packet->removeReference();
		 return;
	}
	if (restartInterval) {
		if (nextRestart.tv_sec==0) {
			DPRINTFL(MSG_VDEBUG, "PCAPExporterPipe::receive(): updating nextRestart");
			nextRestart = packet->timestamp;
			nextRestart.tv_sec += restartInterval;
		} else if (compareTime(nextRestart, packet->timestamp)<0) {
			DPRINTFL(MSG_VDEBUG, "PCAPExporterPipe::receive(): restarting process");

			// we need to unregister our signal handlers, as we get race conditions with the signal handler for restarting the process
			unregisterSignalHandlers();
			stopProcess();
			startProcess();
			registerSignalHandlers();
			DPRINTFL(MSG_VDEBUG, "PCAPExporterPipe::receive(): updating nextRestart");
			nextRestart.tv_sec += ((packet->timestamp.tv_sec-nextRestart.tv_sec)/restartInterval+1)*restartInterval;
		}
	}

	// write packet
	static struct pcap_pkthdr packetHeader;
	packetHeader.ts = packet->timestamp;
	packetHeader.caplen = packet->data_length;
	packetHeader.len = packet->pcapPacketLength;
	struct iovec wvec[2];
	wvec[0].iov_base = &packetHeader;
	wvec[0].iov_len = sizeof(packetHeader);
	wvec[1].iov_base = packet->data;
	wvec[1].iov_len = packetHeader.caplen;
	if (writev(pcapFile, wvec, 2)!=(ssize_t)(sizeof(packetHeader)+packetHeader.caplen)) {
		if (errno==EAGAIN) {
			// pipe is full, drop packet
			statBytesDropped += packet->data_length;
			statPktsDropped++;
		} else
			THROWEXCEPTION("PCAPExporterPipe: failed to write, error %u (%s)", errno, strerror(errno));
	} else {
		statBytesForwarded += packet->data_length;
		statPktsForwarded++;
	}

	packet->removeReference();
	DPRINTFL(MSG_VDEBUG, "PCAPExporterPipe::receive() ended");
}


void PCAPExporterPipe::performShutdown()
{
	unregisterSignalHandlers();
	stopProcess();
}


bool PCAPExporterPipe::getQueueStats(uint32_t* maxsize, uint32_t* cursize)
{
	return false;
}
