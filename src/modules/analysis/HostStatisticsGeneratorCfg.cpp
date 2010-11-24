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

#include "HostStatisticsGeneratorCfg.h"

#include <arpa/inet.h>

HostStatisticsGeneratorCfg::HostStatisticsGeneratorCfg(XMLElement* elem) : CfgHelper<HostStatisticsGenerator, HostStatisticsGeneratorCfg>(elem, "hostStatistics")
{
	if (!elem) return;  // needed because of table inside ConfigManager

	try {
		if (inet_aton(get("subnet").c_str(), (in_addr*)(&ipSubnet))==0)
			THROWEXCEPTION("Invalid subnetwork specified for module hostStatisticsGenerator");
		if (inet_aton(get("mask").c_str(), (in_addr*)(&ipMask))==0)
			THROWEXCEPTION("Invalid subnetwork specified for module hostStatisticsGenerator");
		intervalLength = getInt("intervallength", 1000);
	} catch(IllegalEntry ie) {
		THROWEXCEPTION("Illegal hostStatistics entry in config file");
	}
}

bool HostStatisticsGeneratorCfg::deriveFrom(HostStatisticsGeneratorCfg* old)
{
	return false;
}

HostStatisticsGeneratorCfg* HostStatisticsGeneratorCfg::create(XMLElement* e)
{
	assert(e);
	assert(e->getName() == getName());
	return new HostStatisticsGeneratorCfg(e);
}

HostStatisticsGenerator* HostStatisticsGeneratorCfg::createInstance()
{
	if (!instance) {
		instance = new HostStatisticsGenerator(ipSubnet, ipMask, intervalLength);
	}
	return instance;
}
