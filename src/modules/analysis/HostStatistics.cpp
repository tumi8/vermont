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



HostStatistics::HostStatistics(std::string ipSubnet, std::string addrFilter, std::string logPath, uint16_t logInt)
	: ipSubnet(ipSubnet), addrFilter(addrFilter), logPath(logPath), logInt(logInt)
{
	// check if srcIP or dstIP in the subnet (1.1.1.1/16)
	// split string at the '/'
	size_t found = ipSubnet.find_first_of("/");
	std::string ip_str = ipSubnet.substr(0, found);
	netSize = atoi(ipSubnet.substr(found + 1).c_str());
	netAddr = *(uint32_t *)gethostbyname(ip_str.c_str())->h_addr;
	logTimer = time(NULL);

	// DEBUG
	FILE* logFile = fopen("host_stats.log", "a");
	fprintf(logFile, "netAddr: %u\t", netAddr);
	fprintf(logFile, " - %s\n", IPToString(netAddr).c_str());
	fclose(logFile);
}

void HostStatistics::onDataRecord(IpfixDataRecord* record)
{
	// only treat non-Options Data Records (although we cannot be sure that there is a Flow inside)
	if((record->templateInfo->setId != TemplateInfo::NetflowTemplate) 
		&& (record->templateInfo->setId != TemplateInfo::IpfixTemplate) 
		&& (record->templateInfo->setId != TemplateInfo::IpfixDataTemplate)) {
		record->removeReference();
		return;
	}
	
	Connection conn(record);
	std::map<uint32_t, uint64_t>::iterator it;

	FILE* logFile = fopen("host_stats.log", "a");

	if ((addrFilter == "src") && ((conn.srcIP&netAddr) == netAddr)) {
		it = trafficMap.find(conn.srcIP);
		if (it == trafficMap.end())	{
			trafficMap.insert(pair<uint32_t, uint64_t>(conn.srcIP, ntohl(conn.srcOctets)));
		} else {
			it->second += ntohl(conn.srcOctets);
		}
	} else if ((addrFilter == "dst") && ((conn.dstIP&netAddr) == netAddr)) {
		it = trafficMap.find(conn.dstIP);
		if (it == trafficMap.end()) {
			trafficMap.insert(pair<uint32_t, uint64_t>(conn.dstIP, ntohl(conn.dstOctets)));
		} else {
			it->second += ntohl(conn.dstOctets);
		}
	} else {
		if ((conn.srcIP&netAddr) == netAddr) {
			fprintf(logFile, "Treffer - src\t");
			it = trafficMap.find(conn.srcIP);
			if (it == trafficMap.end())	{
				fprintf(logFile, "- %s\n", IPToString(conn.srcIP).c_str());
				trafficMap.insert(pair<uint32_t, uint64_t>(conn.srcIP, (ntohl(conn.srcOctets) + ntohl(conn.dstOctets))));
			} else {
				fprintf(logFile, "\n");
				it->second += (ntohl(conn.srcOctets) + ntohl(conn.dstOctets));
			}
		} else if ((conn.dstIP&netAddr) == netAddr) {
			fprintf(logFile, "Treffer - dst\t");
			it = trafficMap.find(conn.dstIP);
			if (it == trafficMap.end())	{
				fprintf(logFile, "- %s\n", IPToString(conn.dstIP).c_str());
				trafficMap.insert(pair<uint32_t, uint64_t>(conn.dstIP, (ntohl(conn.srcOctets) + ntohl(conn.dstOctets))));
			} else {
				fprintf(logFile, "\n");
				it->second += (ntohl(conn.srcOctets) + ntohl(conn.dstOctets));
			}
		}
	}
	fclose(logFile);
}

void HostStatistics::onReconfiguration1()
{
	std::map<uint32_t, uint64_t>::iterator it;

	FILE* logFile = fopen(logPath.c_str(), "w");
	// insert current timestamp
	fprintf(logFile, "%d", (int)time(NULL));
	// for each element in ipList, write an entry like: IP:Bytesum
	for (it = trafficMap.begin(); it != trafficMap.end(); it++) {
		fprintf(logFile, " %s:%u", IPToString(it->first).c_str(), (uint32_t)it->second);
	}
	fclose(logFile);
}
