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

#ifndef _PCAP_EXPORTER_PROCESS_BASE_H_
#define _PCAP_EXPORTER_PROCESS_BASE_H_

#include "core/Module.h"

#include <common/msg.h>

#include <string>
#include <ctime>
#include "PCAPExporterBase.h"
#include <common/SignalInterface.h>

class Packet;


/**
 * This class provides basic functions for all PCAPExporters that forward data to processes
*/

class PCAPExporterProcessBase :
	public PCAPExporterBase,
	public SignalInterface
{
public:
	PCAPExporterProcessBase(const std::string& logfile);
	virtual ~PCAPExporterProcessBase();

    virtual void setPipeReaderCmd(const std::string& cmd);
    virtual void setWorkingPath(const std::string& path);
    virtual void setSigKillTimeout(int s);
	virtual void setRestartOnSignal(bool b);
	virtual void setRestartInterval(uint32_t ri);
	virtual void setAppendDate(bool b);
	virtual void handleSigChld(int sig);
	virtual void handleSigPipe(int sig);
	virtual void handleSigUsr2(int sig);
	virtual std::string getStatisticsXML(double interval);

	/**
	 * returns process statistics of the attached process
	 * @returns true if process is running, false if not
	 */
	virtual bool getProcessStatistics(uint32_t& sysjiffies, uint32_t& userjiffies);

	/**
	 * returns total number of dropped and forwarded packets due to overload of the attached process
	 */
	virtual void getPacketStats(uint64_t& droppedpkts, uint64_t& forwpkts);

	/**
	 * returns total number of dropped and forwarded octets due to overload of the attached process
	 */
	virtual void getOctetStats(uint64_t& droppedocts, uint64_t& forwocts);

	virtual bool getQueueStats(uint32_t* maxsize, uint32_t* cursize) = 0;


protected:
	static void* pcapExporterSink(void* data);

	/*name of the optional logfile*/
	std::string logFileName;
	/*startup command for the external program*/
	std::string fifoReaderCmd;
	/*working path for the external command*/
	std::string workingPath;
	/*true if the module is restarting*/
	volatile bool onRestart;
	/*if true, the logfile's name will be appended with the current time/date*/
	bool appenddate;
	/*if true, the external command will be restarted, when SIGUSR2 is
	 * sent to Vermont*/
	bool restartOnSignal;
	/*PID of the external process*/
    int fifoReaderPid;
	/* time gap in seconds between SIGTERM an SIGKILL*/ 
    int sigKillTimeout;
	/* restart counter for the external process*/
	int counter;
	/*time of the last restart of the external command*/
	time_t last_check;
	uint64_t statPktsForwarded;
	uint64_t statBytesForwarded;
	uint64_t statPktsDropped;
	uint64_t statBytesDropped;
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

	/**
	 * file descriptors for our pipe to the process
	 * fd[1]: vermont writes into pipe
	 * fd[0]: child process reads from pipe
	 */
    int fd[2];
	int child_parent_pipe[2];

	virtual void registerSignalHandlers();
	virtual void unregisterSignalHandlers();
	virtual bool isRunning(int pid);
    virtual void kill_pid(int ppid);
    virtual void startProcess() = 0;

    void kill_all(int ppid);
    bool checkint(const char *my_string);
    void parseCommandLine(std::string &cmd, std::vector<std::string> &tokens);
    void redirectLogfile();
};


#endif
