/*
 * VERMONT
 * Copyright (C) 2009 Matthias Segschneider <matthias.segschneider@gmx.de>
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
 */

#ifndef HOSTSTATISTICSCFG_H_
#define HOSTSTATISTICSCFG_H_

#include "core/InfoElementCfg.h"
#include "HostStatistics.h"

#include <string>


class HostStatisticsCfg : public CfgHelper<HostStatistics, HostStatisticsCfg>
{
public:
	friend class ConfigManager;

	std::string getName() { return "hostStatistics"; }

	HostStatistics* createInstance();
	virtual HostStatisticsCfg* create(XMLElement* e);
	virtual bool deriveFrom(HostStatisticsCfg* old);

protected:
	HostStatisticsCfg(XMLElement*);

	std::string getSubnet() { return ipSubnet; }
	std::string getAddrFilter() { return addrFilter; }
	std::string getLogPath() { return logPath; }
	uint16_t getLogIntervall() { return logInt; }

private:
	std::string ipSubnet;
	std::string addrFilter;
	std::string logPath;
	uint16_t logInt;
};

#endif /* HOSTSTATISTICSCFG_H_ */
