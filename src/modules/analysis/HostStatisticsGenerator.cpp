/*
 * VERMONT
 * Copyright (C) 2010 Tobias Limmer <limmer@cs.fau.de>
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


#include "HostStatisticsGenerator.h"
#include "modules/ipfix/Connection.h"
#include "common/Misc.h"
#include "common/Time.h"

#include <netdb.h>


HostStatisticsGenerator* HostStatisticsGeneratorFactory::instance = NULL;

HostStatisticsGenerator* HostStatisticsGeneratorFactory::getInstance()
{
	if (!instance) THROWEXCEPTION("FATAL ERROR: no HostStatisticsGenerator instance set yet!");
	return instance;
}

void HostStatisticsGeneratorFactory::registerInstance(HostStatisticsGenerator* i)
{
	if (instance) THROWEXCEPTION("FATAL ERROR: HostStatisticsGenerator instance already set!");
	instance = i;
}

HostStatistics::HostStatistics(uint64_t octs, uint64_t pkts)
	: lastOctets(0),
	  lastPackets(0),
	  curOctets(octs),
	  curPackets(pkts)
{
}

void HostStatistics::add(uint64_t octs, uint64_t pkts)
{
	curOctets += octs;
	curPackets += pkts;
}

void HostStatistics::commit()
{
	lastOctets = curOctets;
	lastPackets = curPackets;
	curOctets = 0;
	curPackets = 0;
}

/**
 * @param ipSubnet subnet of regarded hosts
 * @param ipMask subnet mask of regarded hosts
 * @param intervalLength length of the interval in milliseconds
 */
HostStatisticsGenerator::HostStatisticsGenerator(uint32_t ipSubnet, uint32_t ipMask, uint32_t intervalLength)
	: ipSubnet(ipSubnet),
	  ipMask(ipMask),
	  zeroOctets(0),
	  intervalLength(intervalLength)
{
	addToCurTime(&intervalEnd, intervalLength);
}


void HostStatisticsGenerator::addHostData(uint32_t ip, uint64_t octs, uint64_t pkts)
{
	std::map<uint32_t, HostStatistics>::iterator it;
	it = trafficMap.find(ip);
	if (it == trafficMap.end())	{
		trafficMap.insert(pair<uint32_t, HostStatistics>(ip, HostStatistics(octs, pkts)));
	} else {
		it->second.add(octs, pkts);
	}
}

void HostStatisticsGenerator::changeInterval()
{
	std::map<uint32_t, HostStatistics>::iterator it;
	uint32_t zerohosts = 0;
	uint64_t zeroocts = 0;
	for (it=trafficMap.begin(); it!=trafficMap.end(); it++) {
		if (it->second.lastOctets==0) {
			zerohosts++;
			zeroocts += it->second.curOctets;
		}
		it->second.commit();
	}
	if (zerohosts>0) zeroOctets = (double)zeroocts/zerohosts*HSG_DEFAULT_ZEROOCTETSALPHA+(1.0-HSG_DEFAULT_ZEROOCTETSALPHA)*zeroOctets;
	else zeroOctets = (1.0-HSG_DEFAULT_ZEROOCTETSALPHA)*zeroOctets;
	msg(MSG_DEBUG, "zeroocts=%llu, zerohosts=%u", zeroocts, zerohosts);
	msg(MSG_DEBUG, "HostStatisticsGenerator: average traffic by hosts with no traffic in last interval: %llu bytes", zeroOctets);
}

void HostStatisticsGenerator::onDataRecord(IpfixDataRecord* record)
{
	// only treat non-Options Data Records (although we cannot be sure that there is a Flow inside)
	if((record->templateInfo->setId != TemplateInfo::NetflowTemplate) 
		&& (record->templateInfo->setId != TemplateInfo::IpfixTemplate) 
		&& (record->templateInfo->setId != TemplateInfo::IpfixDataTemplate)) {
		record->removeReference();
		return;
	}
	
	Connection conn(record);

	uint32_t srcip = ntohl(conn.srcIP);
	uint32_t dstip = ntohl(conn.dstIP);

	if ((srcip&ipMask)==ipSubnet) {
		addHostData(srcip, ntohll(conn.srcOctets)+ntohll(conn.dstOctets), ntohll(conn.srcPackets)+ntohll(conn.dstPackets));
	}
	if ((dstip&ipMask)==ipSubnet) {
		addHostData(dstip, ntohll(conn.srcOctets)+ntohll(conn.dstOctets), ntohll(conn.srcPackets)+ntohll(conn.dstPackets));
	}
	record->removeReference();

	// check if we need to change intervals and move our data around
	struct timeval curtime;
	gettimeofday(&curtime, 0);
	if (compareTime(curtime, intervalEnd)>0) {
		changeInterval();
		intervalEnd = curtime;
		intervalEnd.tv_sec += intervalLength/1000;
		intervalEnd.tv_usec += intervalLength%1000;
	}
}

/**
 * @param octets the given IP's octets that were transferred in the last interval
 * @returns true if host was watched, else false
 */
bool HostStatisticsGenerator::getOctets(uint32_t ip, uint64_t& octets)
{
	std::map<uint32_t, HostStatistics>::iterator it;
	it = trafficMap.find(ip);
	if (it != trafficMap.end())	{
		octets = it->second.lastOctets;
		return true;
	} else {
		// not found
		return false;
	}
}

void HostStatisticsGenerator::getWatchedSubnet(uint32_t& subnet, uint32_t& mask)
{
	subnet = ipSubnet;
	mask = ipMask;
}

uint64_t HostStatisticsGenerator::getZeroOctets()
{
	return zeroOctets;
}
