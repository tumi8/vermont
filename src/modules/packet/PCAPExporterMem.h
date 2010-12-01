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

#ifndef _PCAP_EXPORTER_MEM_H_
#define _PCAP_EXPORTER_MEM_H_

#include "core/Module.h"

#include <common/msg.h>

#include <string>
#include <pcap.h>
#include <ctime>
#include <mqueue.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/mman.h>

#include "PCAPExporterBase.h"
#include "PCAPExporterPipe.h"
#include <common/SignalInterface.h>

class Packet;

/**
 * This class writes packets in PCAP format into a pipe,
 * allowing another process to read these packets via STDIN.
 * The reader process is started and ended by Vermont.
 * The reader process may be restarted manually by sending SIGUSR2 to Vermont.
*/

class PCAPExporterMem : public PCAPExporterPipe
{
	typedef struct SHMEntry{
		struct daq_pkthdr
		{
			struct timeval ts;      /* Timestamp */
			uint32_t caplen;        /* Length of the portion present */
			uint32_t pktlen;        /* Length of this packet (off wire) */
			int device_index;       /* Index of the receiving interface. */
			uint32_t flags;         /* Flags for the packet (DAQ_PKT_FLAG_*) */
		} packetHeader;
		char data[1600];

	}SHMEntry;


public:
	PCAPExporterMem(const std::string& file);
	~PCAPExporterMem();
  	virtual void receive(Packet* packet);
	virtual void performStart();
	virtual void performShutdown();
	virtual void handleSigChld(int sig);

protected:
    virtual int execCmd(std::string& cmd);
    virtual void startProcess();
    virtual void stopProcess();

private:
	bool writeIntoMemory(Packet *packet);
	void *getNewSharedMemory(int *fd, int size, std::string name);
	//sets up all queue variables 
	void createQueue(int maxEntries);
	//closes shmfd/queuefd and calls shm_unlink()
	int removeSHM(int);
	/**
	 * returns next array position
	 * @param index current array position
	 * @return consecutive array position
	 */
	inline uint32_t next(uint32_t index){
		return (++index % *max);
	}


	// filehandle for the shared memory segment which holds the actual queue
	int shmfd;
	// filehandle for the queue's control variables
	int queuefd;
	SHMEntry *shm_list;
	void *queuevarspointer;
	const int QUEUEENTRIES;
	int packetcount;
	int dropcount;

	/*shared control variables*/
	uint32_t* glob_read;
	uint32_t* glob_write;
	/*consumer’s local variables*/
	uint32_t* localWrite;
	uint32_t* nextRead;
	uint32_t* rBatch;
	/*producer’s local variables*/
	uint32_t* localRead;
	uint32_t* nextWrite;
	uint32_t* wBatch;
	/*constants*/
	uint32_t* max;
	uint32_t* batchSize;

};


#endif
