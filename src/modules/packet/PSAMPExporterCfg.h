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

#ifndef PSAMPEXPORTERCFG_H_
#define PSAMPEXPORTERCFG_H_

#include "core/Cfg.h"
#include "modules/ipfix/CollectorCfg.h"
#include "modules/packet/PSAMPExporterModule.h"

#include <vector>

// forward declarations
class PacketReportingCfg;

class PSAMPExporterCfg
	: public CfgHelper<PSAMPExporterModule, PSAMPExporterCfg>
{
	friend class ConfigManager;
public:
	virtual ~PSAMPExporterCfg();

	virtual PSAMPExporterCfg* create(XMLElement* elem);
	
	virtual PSAMPExporterModule* createInstance();
	
	bool deriveFrom(PSAMPExporterCfg* old);

protected:
	PSAMPExporterCfg(XMLElement* elem); 

private:
	/** this holds the info for all the hosts we want to export to */
	std::vector<CollectorCfg*> collectors;

	/** udpTemplateManagement */
	unsigned templateRefreshTime;
	/* unsigned templateRefreshRate; */ /* TODO */

	/** packet restrictions */
	uint16_t maxPacketSize;
	unsigned exportDelay;
	
	int recordsPerPacket;
	int recordLength;

	uint32_t observationDomainId;
	
	PacketReportingCfg* reporting;
};


#endif /*PSAMPEXPORTERCFG_H_*/
