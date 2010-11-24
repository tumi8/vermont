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
	float priority;
	float weight;

	int16_t assignedIdsId; /**< -1 if not assigned */
	uint64_t nextTraffic; // estimation!
	uint64_t lastTraffic;

	struct timeval startMon; /**< time when host was assigned for monitoring */

	HostData(float p, float w)
		: priority(p), weight(w), assignedIdsId(-1), nextTraffic(0), lastTraffic(0)
	{
		startMon.tv_sec = 0;
		startMon.tv_usec = 0;
	}

	bool belowMinMonTime(struct timeval& tv, struct timeval& minmontime);
};

struct PriorityNetConfig
{
	uint32_t subnet;
	uint32_t mask;
	uint8_t maskbits;
	uint32_t hostcount;
	float weight;

	PriorityNetConfig(uint32_t subnet, uint32_t mask, uint8_t maskbits, float weight);
};

struct IDSData
{
	uint64_t maxOctets;		/**< maximum octets that can be monitored by IDS in one interval */
	list<pair<uint32_t, HostData*> > hosts; /**< hosts assigned to ids */

	IDSData(uint32_t maxoctets)
		: maxOctets(maxoctets)
	{}
};


class PriorityPacketSelector : public BasePacketSelector
{
public:
	PriorityPacketSelector(list<PriorityNetConfig>& pnc, float startprio, struct timeval minmontime);
	virtual int decide(Packet *p);
	virtual void updateData(list<IDSLoadStatistics>& lstats);
	virtual void setQueueCount(uint32_t n);

private:
	static const uint32_t WARN_HOSTCOUNT;

	list<PriorityNetConfig> config;
	map<uint32_t, HostData> hosts;
	float startPrio;
	uint32_t hostCount;
	IpPacketSelector ipSelector;
	uint32_t maxHostPrioChange;	/**< called 'm' in paper */
	float prioSum;
	struct timeval minMonTime; /**< minimal monitoring time in milliseconds */

	list<pair<uint32_t, HostData*> > restHosts; /**< hosts that are currently not monitored */
	vector<IDSData> ids;

	uint32_t insertSubnet(uint32_t subnet, uint8_t maskbits, float weight);
	void updateTrafficEstimation();
	void calcMaxHostPrioChange();
	void updatePriorities();
	void assignHosts2IDS(list<IDSLoadStatistics>& lstats);
	void setIpConfig();
};


#endif
