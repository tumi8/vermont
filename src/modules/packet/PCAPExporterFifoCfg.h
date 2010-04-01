/*
 * Vermont Configuration Subsystem
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

#ifndef PCAPEXPORTERFIFOCFG_H_
#define PCAPEXPORTERFIFOCFG_H_

#include "core/Cfg.h"
#include "modules/packet/PCAPExporterFifo.h"
#include "modules/packet/PCAPExporterBase.h"

#include <vector>


class PCAPExporterFifoCfg
	: public CfgHelper<PCAPExporterFifo, PCAPExporterFifoCfg>
{
	friend class ConfigManager;
public:
	virtual ~PCAPExporterFifoCfg();

	virtual PCAPExporterFifoCfg* create(XMLElement* elem);
	
	virtual PCAPExporterFifo* createInstance();
	
	bool deriveFrom(PCAPExporterFifoCfg* old);

protected:
	PCAPExporterFifoCfg(XMLElement* elem); 

private:
	std::string logFileName;
    std::string fifoReaderCmd;
    int sigkilltimeout;
	int link_type;
	int snaplen;
};


#endif /*PCAP_EXPORTERFILECFG_H_*/
