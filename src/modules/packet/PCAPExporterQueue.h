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

#ifndef _PCAP_EXPORTER_QUEUE_H_
#define _PCAP_EXPORTER_QUEUE_H_

#include "core/Module.h"

#include <common/msg.h>

#include <string>
#include <pcap.h>
#include <ctime>
#include <mqueue.h>
#include <sys/time.h>
#include <sys/resource.h>
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

class PCAPExporterQueue : public PCAPExporterPipe
{
	struct daq_pkthdr
	{
		struct timeval ts;      /* Timestamp */
		uint32_t caplen;        /* Length of the portion present */
		uint32_t pktlen;        /* Length of this packet (off wire) */
		int device_index;       /* Index of the receiving interface. */
		uint32_t flags;         /* Flags for the packet (DAQ_PKT_FLAG_*) */
	} ;

	struct QueueMessage {
		struct daq_pkthdr packetHeader;
		char data[PCAP_MAX_CAPTURE_LENGTH];
	};


public:
	PCAPExporterQueue(const std::string& file, uint32_t maxqueuemsgs);
	~PCAPExporterQueue();
  	virtual void receive(Packet* packet);
	virtual void performStart();
	virtual void performShutdown();
	virtual void handleSigChld(int sig);
	std::string getStatisticsXML(double interval);

protected:
    virtual int execCmd(std::string& cmd);
    virtual void startProcess();
    virtual void stopProcess();

private:
	bool dumpIntoQueue(Packet *packet);
	int closeQueue(int, int);
	mqd_t queuedes;
	uint32_t maxQueueMsgs;
	uint32_t freeEntries; /**< free entries in queue - this is a pessimistic variable! */
};


#endif
