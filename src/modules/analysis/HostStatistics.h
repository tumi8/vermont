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

#ifndef HOSTSTATISTICS_H_
#define HOSTSTATISTICS_H_

#include <time.h>
#include <map>

#include "modules/ipfix/IpfixRecordDestination.h"
#include "Host.h"


class HostStatistics : public IpfixRecordDestination, public Module, public Source<NullEmitable*>
{
public:
	HostStatistics(std::string ipSubnet, std::string addrFilter, std::string logPath, uint16_t logInt);
	virtual ~HostStatistics();
	void onDataRecord(IpfixDataRecord* record);

	virtual void onReconfiguration1();

private:
	void dumpStatistics();

	static InstanceManager<Host> hostManager;

	std::string ipSubnet;
	std::string addrFilter;
	std::string logPath;
	uint16_t logInt;
	uint32_t netAddr;
	uint8_t netSize;
	time_t logTimer;
	typedef std::map<uint32_t, Host*> HostMap;
	HostMap hostMap;
};

#endif /* HOSTSTATISTICS_H_ */
