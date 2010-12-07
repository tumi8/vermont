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
#include "PCAPExporterProcessBase.h"

#include <common/msg.h>

#include <string>
#include <ctime>
#include "PCAPExporterBase.h"

class Packet;



/**
 * This class writes packets in PCAP format into a pipe,
 * allowing another process to read these packets via STDIN.
 * The reader process is started and ended by Vermont.
 * The reader process may be restarted manually by sending SIGUSR2 to Vermont.
*/

class PCAPExporterPipe :
	public Module,
	public Destination<Packet *>,
	public Source<Packet *>,
	public PCAPExporterProcessBase
{
public:
	PCAPExporterPipe(const std::string& file);
	virtual ~PCAPExporterPipe();
	virtual void receive(Packet* packet);
	
	virtual bool getQueueStats(uint32_t* maxsize, uint32_t* cursize);

protected:
	static void* pcapExporterSink(void* data);
	virtual void performStart();
	virtual void performShutdown();
    virtual void startProcess();
    virtual void stopProcess();
    virtual int execCmd(std::string& cmd);

	int pcapFile;

};


#endif
