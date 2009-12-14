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

#ifndef REPORTEDIECFG_H_
#define REPORTEDIECFG_H_

#include "core/Cfg.h"

#include <string>

// forward declaration
class InfoElementCfg;
class Template;

class PacketReportingCfg
	: public CfgBase
{
public:
	PacketReportingCfg(XMLElement* elem);
	~PacketReportingCfg();

	Template* getTemplate();

	uint16_t getRecordLength();

	uint16_t getRecordsVariableLen();

private:
        uint16_t recordVLFields;
        uint16_t recordLength;

	unsigned templateId;
	std::vector<InfoElementCfg*> exportedFields;
	Template* t;
};

#endif /*REPORTEDIECFG_H_*/
