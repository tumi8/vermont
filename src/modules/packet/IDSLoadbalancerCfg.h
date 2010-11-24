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

#ifndef IDS_LOADBALANCER_CFG_HPP
#define IDS_LOADBALANCER_CFG_HPP

#include "core/Cfg.h"
#include "modules/packet/IDSLoadbalancer.hpp"
#include "modules/packet/PCAPExporterBase.h"

#include <vector>
#include <map>

class IDSLoadbalancerCfg
	: public CfgHelper<IDSLoadbalancer, IDSLoadbalancerCfg>
{
	friend class ConfigManager;
public:
	virtual ~IDSLoadbalancerCfg();

	virtual IDSLoadbalancerCfg* create(XMLElement* elem);

	virtual IDSLoadbalancer* createInstance();

	bool deriveFrom(IDSLoadbalancerCfg* old);

protected:
	IDSLoadbalancerCfg(XMLElement* elem);

private:
	std::map<uint32_t, int> dst;
	std::map<uint32_t, int> src;
	int _queuecount;
	BasePacketSelector* selector;
	uint32_t updateInterval;

	uint32_t parseIp(std::string &);

};


#endif 
