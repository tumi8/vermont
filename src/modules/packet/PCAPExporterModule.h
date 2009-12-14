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

#ifndef _PCAP_EXPORTER_MODULE_H_
#define _PCAP_EXPORTER_MODULE_H_

#include "core/Module.h"

#include <common/msg.h>

#include <string>
#include <pcap.h>

class Packet;

class PCAPExporterModule : public Module, public Destination<Packet*>, public Source<Packet*>
{
public:
	PCAPExporterModule(const std::string& file);
	~PCAPExporterModule();

	virtual void receive(Packet* packet);
	virtual void performStart();
	virtual void performShutdown();

	void setDataLinkType(int type);
	void setSnaplen(int len);

private:
	static void* pcapExporterSink(void* data);

	std::string fileName;
	pcap_t* dummy;
	pcap_dumper_t* dumper;
	int link_type;
	int snaplen;
};

#endif
