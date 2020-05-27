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


#include "HostStatistics.h"
#include "modules/ipfix/Connection.h"
#include "common/Misc.h"

#include <netdb.h>
#include <fstream>

InstanceManager<Host> HostStatistics::hostManager("Host");

HostStatistics::HostStatistics(std::string ipSubnet, std::string addrFilter, std::string logPath)
	: ipSubnet(ipSubnet), addrFilter(addrFilter), logPath(logPath)
{
	// check if srcIP or dstIP in the subnet (1.1.1.1/16)
	// split string at the '/'
	size_t found = ipSubnet.find_first_of("/");
	if (found == string::npos){
		THROWEXCEPTION("HostStatistics: Received invalid subnet: '%s'", ipSubnet.c_str());
	}
	std::string ip_str = ipSubnet.substr(0, found);
	std::string netmask = ipSubnet.substr(found + 1);
	try {
		netSize = std::stoi(netmask);
		if (netSize > 32)
			throw std::runtime_error("Invalid subnet masksize");
	} catch (...) {
		THROWEXCEPTION("HostStatistics: Invalid subnet mask in subnet: '%s'", netmask.c_str());
	}
	struct hostent* host = gethostbyname(ip_str.c_str());
	if (host == NULL) {
		THROWEXCEPTION("HostStatistics: Invalid IP address in subnet: '%s'", ip_str.c_str());
	}
	netAddr = *(uint32_t *)host->h_addr;
	logTimer = time(NULL);
}

HostStatistics::~HostStatistics()
{
	dumpStatistics();
}

void HostStatistics::onDataRecord(IpfixDataRecord* record)
{
	// only treat non-Options Data Records (although we cannot be sure that there is a Flow inside)
	if((record->templateInfo->setId != TemplateInfo::NetflowTemplate) 
		&& (record->templateInfo->setId != TemplateInfo::IpfixTemplate)) {
		record->removeReference();
		return;
	}
	
	Connection conn(record);
	HostMap::iterator it;

//	FILE* logFile = fopen("host_stats.log", "a");

	if ((addrFilter == "src" || addrFilter == "both") && ((conn.srcIP&netAddr) == netAddr)) {
		it = hostMap.find(conn.srcIP);
		if (it == hostMap.end())	{
			Host* h = hostManager.getNewInstance();
			h->setIP(conn.srcIP);
			it = hostMap.insert(pair<uint32_t, Host*>(conn.srcIP, h)).first;
		}
		it->second->addConnection(&conn);
	} 
	if ((addrFilter == "dst" || addrFilter == "both") && ((conn.dstIP&netAddr) == netAddr)) {
		it = hostMap.find(conn.dstIP);
		if (it == hostMap.end()) {
			Host* h = hostManager.getNewInstance();
			h->setIP(conn.dstIP);
			it = hostMap.insert(pair<uint32_t, Host*>(conn.dstIP, h)).first;
		} 
		it->second->addConnection(&conn);
	}
}

void HostStatistics::onReconfiguration1()
{
	dumpStatistics();
}

void HostStatistics::dumpStatistics()
{
	HostMap::iterator it;
	std::fstream outfile(logPath.c_str(), fstream::out);
	outfile << "# ip answeredFlows unansweredFlows sentBytes sentPackets recBytes recPackets recHighPorts sentHighports recLowPorts sentLowPorts" << std::endl;

	// for each element in ipList, write an entry like: IP:Bytesum
	for (it = hostMap.begin(); it != hostMap.end(); it++) {
		Host* h = it->second;
		outfile << IPToString(it->first).c_str() << " "
			<< h->answeredFlows << " "
			<< h->unansweredFlows << " "
			<< h->sentBytes << " " 
			<< h->sentPackets << " "
			<< h->recBytes << " "
			<< h->recPackets << " "
			<< h->recHighPorts << " "
			<< h->sentHighPorts << " "
			<< h->recLowPorts << " " 
			<< h->sentLowPorts << std::endl;
	}
}
