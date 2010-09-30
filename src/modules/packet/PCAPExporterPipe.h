/*
 * Vermont PCAP Exporter
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

#ifndef _PCAP_EXPORTER_FIFO_H_
#define _PCAP_EXPORTER_FIFO_H_

#include "core/Module.h"

#include <common/msg.h>

#include <string>
#include <pcap.h>
#include <ctime>
#include "PCAPExporterBase.h"
#include <common/SignalInterface.h>

class Packet;

/**
 * This class writes packets in PCAP format into a pipe,
 * allowing another process to read these packets via STDIN.
 * The reader process is started and ended by Vermont.
 * The reader process may be restarted manually by sending SIGUSR2 to Vermont.
*/

class PCAPExporterPipe : public Module, public Destination<Packet *>, public Source<Packet *>, public PCAPExporterBase, public SignalInterface
{
public:
	PCAPExporterPipe(const std::string& file);
	~PCAPExporterPipe();
    void setPipeReaderCmd(const std::string& cmd);
    void setWorkingPath(const std::string& path);
    void setSigKillTimeout(int s);
	void setRestartOnSignal(bool b);
	void setRestartInterval(uint32_t ri);
	void setAppendDate(bool b);
	virtual void handleSigChld(int sig);
	virtual void handleSigPipe(int sig);
	virtual void handleSigUsr2(int sig);
	bool isRunning(int pid);
    int execCmd(std::string& cmd);
    void kill_all(int ppid);
    void kill_pid(int ppid);
    void startProcess();
    void stopProcess();
    bool checkint(const char *my_string);

//    virtual void postReconfiguration();
//    virtual void preReconfiguration();
//    virtual void onReconfiguration1();
//    virtual void onReconfiguration2();
	virtual void receive(Packet* packet);
	virtual void performStart();
	virtual void performShutdown();
	virtual std::string getStatisticsXML(double interval);

private:
	static void* pcapExporterSink(void* data);

	std::string logFileName;
	std::string fifoReaderCmd;
	std::string workingPath;
	volatile bool onRestart;
	bool appenddate;
	bool restartOnSignal;
    int fifoReaderPid;
	pcap_t* dummy;
    int sigKillTimeout;
	int counter;
	time_t last_check;
	/**
	 * file descriptors for our pipe to the process
	 * fd[1]: vermont writes into pipe
	 * fd[0]: child process reads from pipe
	 */
    int fd[2];
	FILE* pcapFile;
	uint64_t statPktsForwarded;
	uint64_t statBytesForwarded;
	/**
	 * restart interval of the process in seconds
	 * time is measured by the timestamps of the incoming packets
	 * if 0, the process will not be automatically restarted
	 * if >0, the process will be regularly restarted after the given time
	 */
	uint32_t restartInterval;
	/**
	 * next time the process will be restarted (used for parameter restartInterval)
	 */
	struct timeval nextRestart;

	void registerSignalHandlers();
	void unregisterSignalHandlers();
};


#endif
