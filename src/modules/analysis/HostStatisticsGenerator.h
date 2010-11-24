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

#ifndef HOSTSTATISTICSGENERATOR_H_
#define HOSTSTATISTICSGENERATOR_H_

#include <time.h>
#include <map>

#include "modules/ipfix/IpfixRecordDestination.h"


struct HostStatistics
{
	HostStatistics(uint64_t octs, uint64_t pkts);
	void add(uint64_t octs, uint64_t pkts);
	void commit();

	// aggregate values of last interval
	uint64_t lastOctets;
	uint64_t lastPackets;

	// aggregate values of current interval - ATTENTION: will increase until interval has ended!
	uint64_t curOctets;
	uint64_t curPackets;
};


/**
 * Generates traffic statistics for all hosts in the given subnet
 *
 * We assume that the regarded local subnetwork with hosts is only on one side of the monitored link,
 * and we do not observer traffic exchanged between these local hosts. Else each host's transferred data
 * would not necessarily be disjoint!
 */
class HostStatisticsGenerator : public IpfixRecordDestination, public Module, public Source<NullEmitable*>
{
public:
	HostStatisticsGenerator(uint32_t ipSubnet, uint32_t ipMask, uint32_t intervalLength);
	void onDataRecord(IpfixDataRecord* record);



private:
	uint32_t ipSubnet;
	uint32_t ipMask;
	std::map<uint32_t, HostStatistics> trafficMap;
	struct timeval intervalEnd;
	uint32_t intervalLength; // interval length in milliseconds

	void addHostData(uint32_t ip, uint64_t pkts, uint64_t octs);
	void changeInterval();
};



/**
 * a small hack to be able to access the instance of HostStatisticsGenerator from
 * PrioritySystem
 * ATTENTION: we assume that only ONE HostStatisticsGenerator is available as module within Vermont
 */
class HostStatisticsGeneratorFactory
{
public:
	static HostStatisticsGenerator* getInstance();
	static void registerInstance(HostStatisticsGenerator* i);

private:
	static HostStatisticsGenerator* instance;
};

#endif /* HOSTSTATISTICSGENERATOR_H_ */
