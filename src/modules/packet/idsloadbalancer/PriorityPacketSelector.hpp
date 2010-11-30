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

#if !defined(PRIORITYSYSTEM_H__)
#define PRIORITYSYSTEM_H__


#include "BasePacketSelector.hpp"
#include "IpPacketSelector.hpp"

#include <map>
#include <vector>

using namespace std;


struct HostData
{
	double priority;
	double weight;

	uint32_t ip;
	int16_t assignedIdsId; /**< -1 if not assigned */
	uint64_t nextTraffic; // estimation!
	uint64_t lastTraffic;

	struct timeval startMon; /**< time when host was assigned for monitoring */

	HostData(uint32_t ip, double p, double w);
	bool belowMinMonTime(struct timeval& tv, struct timeval& minmontime);
};

struct PriorityNetConfig
{
	uint32_t subnet;
	uint32_t mask;
	uint8_t maskbits;
	uint32_t hostcount;
	double weight;

	PriorityNetConfig(uint32_t subnet, uint32_t mask, uint8_t maskbits, double weight);
};

struct IDSData
{
	uint64_t maxOctets;		/**< maximum octets that can be monitored by IDS in one interval */
	bool slowStart;			/**< determines whether we are in first phase of IDS max. rate detection */
	list<HostData*> hosts; /**< hosts assigned to ids */
	uint32_t hostcount;

	uint64_t lastForwOct;
	uint64_t curForwOct;
	uint64_t lastForwPkt;
	uint64_t curForwPkt;

	uint64_t curDropOct;
	uint64_t lastDropOct;
	uint64_t curDropPkt;
	uint64_t lastDropPkt;

	IDSData(uint32_t maxoctets)
		: maxOctets(maxoctets),
		  slowStart(true),
		  hostcount(0),
		  lastForwOct(0),
		  curForwOct(0),
		  lastForwPkt(0),
		  curForwPkt(0),
		  curDropOct(0),
		  lastDropOct(0),
		  curDropPkt(0),
		  lastDropPkt(0)
	{}
};


class PriorityPacketSelector : public BasePacketSelector
{
public:
	PriorityPacketSelector(list<PriorityNetConfig>& pnc, double startprio, struct timeval minmontime);
	virtual ~PriorityPacketSelector();
	virtual int decide(Packet *p);
	virtual void updateData(list<IDSLoadStatistics>& lstats);
	virtual void setQueueCount(uint32_t n);
	virtual void setUpdateInterval(uint32_t ms);
	virtual void start();
	virtual void stop();

private:
	static const uint32_t WARN_HOSTCOUNT;

	list<PriorityNetConfig> config;
	list<HostData*> hosts;
	uint32_t hostCount;
	map<uint32_t, HostData*> ip2host;
	double startPrio;
	IpPacketSelector ipSelector;
	uint32_t maxHostPrioChange;	/**< called 'm' in paper */
	double prioSum;
	struct timeval minMonTime; /**< minimal monitoring time in milliseconds */
	uint64_t discardOctets;
	struct timeval startTime;
	uint32_t updateInterval; /**< update interval in ms */

	list<HostData*> restHosts; /**< hosts that are currently not monitored */
	vector<IDSData> ids;

	uint32_t insertSubnet(uint32_t subnet, uint8_t maskbits, double weight);
	void updateTrafficEstimation();
	void calcMaxHostPrioChange();
	void updatePriorities();
	void assignHosts2IDS();
	void setIpConfig();
	void updateIDSMaxRate();
};


#endif
