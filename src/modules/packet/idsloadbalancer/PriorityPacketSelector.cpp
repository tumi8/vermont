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


#include "PriorityPacketSelector.hpp"

#include "modules/analysis/HostStatisticsGenerator.h"
#include "common/Time.h"

#include <limits>


const uint32_t PriorityPacketSelector::WARN_HOSTCOUNT = 1<<22;


//#define PPDPRINTFL(lvl, fmt, args...)  msg2(__LINE__, __FILE__, __PRETTY_FUNCTION__, __func__, lvl, fmt, ##args)
#define PPDPRINTFL(lvl, fmt, args...)


HostData::HostData(uint32_t ip, double p, double w)
		: priority(p),
		  weight(w),
		  ip(ip),
		  assignedIdsId(-1),
		  nextTraffic(0),
		  lastTraffic(0)
{
	startMon.tv_sec = 0;
	startMon.tv_usec = 0;
}

/**
 * WARNING: does not set hostcount by itself, it is set by PriorityPacketSelector
 */
PriorityNetConfig::PriorityNetConfig(uint32_t subnet, uint32_t mask, uint8_t maskbits, double weight)
	: subnet(subnet), mask(mask), maskbits(maskbits), hostcount(0), weight(weight)
{
}

bool HostData::belowMinMonTime(struct timeval& tv, struct timeval& minmontime)
{
	struct timeval diff;
	timeval_subtract(&diff, &tv, &startMon);
	return compareTime(minmontime, diff)>0;
}

/**
 * @param pnc expected in order of decreasing bits in the mask!
 */
PriorityPacketSelector::PriorityPacketSelector(list<PriorityNetConfig>& pnc, double startprio, struct timeval minmontime)
	: BasePacketSelector("PriorityPacketSelector"),
	  config(pnc),
	  hostCount(0),
	  startPrio(startprio),
	  minMonTime(minmontime),
	  discardOctets(0),
	  updateInterval(0)
{
	msg(MSG_INFO, "PriorityPacketSelector: Creating host priority entries ...");

	// create host entries
	for (list<PriorityNetConfig>::iterator citer = config.begin(); citer!=config.end(); citer++) {
		citer->hostcount = insertSubnet(citer->subnet, citer->maskbits, citer->weight);
	}
	prioSum = startprio*hostCount;
	msg(MSG_INFO, "PriorityPacketSelector: Created %u entries with start priority %.2f", hostCount, startprio);

	startTime.tv_sec = 0;
	startTime.tv_usec = 0;

	calcMaxHostPrioChange();
}

PriorityPacketSelector::~PriorityPacketSelector()
{
	while (!hosts.empty()) {
		delete hosts.front();
		hosts.pop_front();
	}
}

void PriorityPacketSelector::setQueueCount(uint32_t n)
{
	BasePacketSelector::setQueueCount(n);
	for (uint32_t i=0; i<numberOfQueues; i++) {
		ids.push_back(IDSData(100000));
	}
}


/**
 * @returns number of generated host entries
 */
uint32_t PriorityPacketSelector::insertSubnet(uint32_t subnet, uint8_t maskbits, double weight)
{
	if (maskbits==32) {
		// insert single host
		if (ip2host.find(subnet)==ip2host.end()) {
			HostData* hd = new HostData(subnet, startPrio, weight);
			hosts.push_back(hd);
			ip2host[subnet] = hd;
			restHosts.push_back(hd);
			hostCount++;
		}
		return 1;

	} else {
		// iterate through all possible IP values
		bool warningdisp = false;
		uint32_t hcount = 0;
		for (uint32_t i=1; i<(1u<<(32-maskbits))-1; i++) {
			uint32_t ip = i | subnet;
			if (ip2host.find(ip)==ip2host.end()) {
				HostData* hd = new HostData(ip, startPrio, weight);
				hosts.push_back(hd);
				ip2host[ip] = hd;
				restHosts.push_back(hd);
				hostCount++;
				hcount++;
				if (hostCount>WARN_HOSTCOUNT && !warningdisp) {
					msg(MSG_DIALOG, "PriorityPacketSelector: Warning, number of prioritized hosts is greated than %u!", WARN_HOSTCOUNT);
					warningdisp = true;
				}
			}
		}
		return hcount;
	}
}


/**
 * calculate "m", the maximum number of hosts whose priority may be changed because of monitoring
 */
void PriorityPacketSelector::calcMaxHostPrioChange()
{
	// generate subnet list with ascending weights
	list<PriorityNetConfig*> ascw;
	list<PriorityNetConfig*> temp;
	for (list<PriorityNetConfig>::iterator citer = config.begin(); citer!=config.end(); citer++) {
		temp.push_back(&(*citer));
	}
	while (!temp.empty()) {
		list<PriorityNetConfig*>::iterator maxw = temp.begin();
		for (list<PriorityNetConfig*>::iterator iter = ++temp.begin(); iter!=temp.end(); iter++) {
			if ((*maxw)->weight>(*iter)->weight) {
				maxw = iter;
			}
		}
		ascw.push_back(*maxw);
		temp.erase(maxw);
	}

	maxHostPrioChange = 0;
	double invmaxw = 1.0/ascw.back()->weight;
	double minwsum = 0; // M_minw(r,m)^T/W (temporary value)
	for (list<PriorityNetConfig*>::iterator iter = ascw.begin(); iter!=ascw.end(); iter++) {
		if (invmaxw<=((double)hostCount/(minwsum+(*iter)->hostcount/(*iter)->weight))) {
			// portion of the hosts in this subnet is needed to fulfil criteria
			maxHostPrioChange += (uint32_t)((*iter)->weight*((double)hostCount*invmaxw)-minwsum);
			break;
		} else {
			// invmaxw is not reached yet, add hosts to temporary value
			minwsum += (*iter)->hostcount/(*iter)->weight;
			maxHostPrioChange += (*iter)->hostcount;
		}
	}
	msg(MSG_INFO, "PriorityPacketSelector: maxHostPrioChange=%u", maxHostPrioChange);
	if (maxHostPrioChange==0) {
		THROWEXCEPTION("PriorityPacketSelector: due to the maximum weight assigned to at least one host, this priority system does not work. See section 'extreme cases' in paper.");
	}
}


int PriorityPacketSelector::decide(Packet *p)
{
	int qid = ipSelector.decide(p);
	// we do not need to collect statistics here, this is done by the succeeding modules
	//if (qid>=0)	ids[qid].curOctets += p->data_length;
	if (qid<0) discardOctets += p->data_length;
	return qid;
}

void PriorityPacketSelector::updateIDSMaxRate()
{
	float slowstartratio = 2;
	float adaptratio = 1.1;

	// if we just started the process, do nothing
	struct timeval curtime;
	struct timeval diff;
	gettimeofday(&curtime, 0);
	timeval_subtract(&diff, &curtime, &startTime);
	if (diff.tv_sec<PPS_DEFAULT_MINSTARTDELAY) return;

	for (uint32_t i=0; i<numberOfQueues; i++) {
		uint64_t oldmax = ids[i].maxOctets;

		if (ids[i].slowStart) {
			// roughly try to determine maximum IDS speed
			if (ids[i].curDropOct>0) {
				if (ids[i].curForwOct<ids[i].maxOctets*2) {
					ids[i].slowStart = false;
					ids[i].maxOctets = (float)ids[i].maxOctets/adaptratio;
				} else {
					if (ids[i].curForwOct>ids[i].maxOctets/2)
						ids[i].maxOctets = slowstartratio*(float)ids[i].maxOctets;
				}
			} else {
				if (ids[i].curForwOct>ids[i].maxOctets/2)
					ids[i].maxOctets = slowstartratio*(float)ids[i].maxOctets;
			}
		} else {
			if (ids[i].curDropOct>0) {
				if (ids[i].curForwOct<ids[i].maxOctets)
					ids[i].maxOctets = (float)ids[i].maxOctets/adaptratio;
			} else if (ids[i].curForwOct>ids[i].maxOctets/2) {
				ids[i].maxOctets = (float)ids[i].maxOctets*adaptratio;
			}
		}
		msg(MSG_DEBUG, "PriorityPacketSelector: IDS %u maximum octets - old: %llu, new: %llu", i, oldmax, ids[i].maxOctets);
	}
}


void PriorityPacketSelector::updateTrafficEstimation()
{
	msg(MSG_DEBUG, "PriorityPacketSelector: updating host traffic estimation");
	HostStatisticsGenerator* hoststats = HostStatisticsGeneratorFactory::getInstance();
	uint32_t hsglength = hoststats->getIntervalLength();
	double ratio = (double)updateInterval/hsglength;
	uint64_t zerooctets = ratio*hoststats->getZeroOctets();
	for (list<HostData*>::iterator iter = hosts.begin(); iter!=hosts.end(); iter++) {
		HostData* host = *iter;
		uint64_t octets;
		if (hoststats->getOctets(host->ip, octets)) {
			octets *= ratio;
			host->lastTraffic = octets;
			if (octets==0) host->nextTraffic = zerooctets;
			else host->nextTraffic = octets;
		} else {
			// we do not have any new data for this host ...
			host->lastTraffic = 0;
			host->nextTraffic = zerooctets;
		}
	}
}


bool greaterpriothan(HostData* i, HostData* j)
{
	return i->priority>j->priority;
}


void PriorityPacketSelector::updatePriorities()
{
	msg(MSG_DEBUG, "PriorityPacketSelector: updating host priorities");
	// sort hosts by priority
	vector<HostData*> sortedhosts;
	for (uint32_t i=0; i<numberOfQueues; i++) {
		for (list<HostData*>::iterator iter = ids[i].hosts.begin(); iter!=ids[i].hosts.end(); iter++) {
			HostData* hd = *iter;
			sortedhosts.push_back(hd);
		}
	}
	sort(sortedhosts.begin(), sortedhosts.end(), greaterpriothan);

	// update priorities of monitored hosts
	double curpriosum = 0;
	uint32_t count = 0;
	for (vector<HostData*>::iterator iter=sortedhosts.begin(); iter!=sortedhosts.end(); iter++) {
		if (count<=maxHostPrioChange) {
			PPDPRINTFL(MSG_VDEBUG, "PriorityPacketSelector: old priority of monitored host %s: %.2f", IPToString(htonl((*iter)->ip)).c_str(), (double)(*iter)->priority);
			(*iter)->priority -= 1.0/(*iter)->weight;
		}
		curpriosum += (*iter)->priority;
		count++;
	}

	// build sum of priorities
	for (list<HostData*>::iterator iter=restHosts.begin(); iter!=restHosts.end(); iter++) {
		curpriosum += (*iter)->priority;
		PPDPRINTFL(MSG_VDEBUG, "PriorityPacketSelector: old priority of non-monitored host %s: %.2f", IPToString(htonl((*iter)->ip)).c_str(), (double)(*iter)->priority);
	}
	PPDPRINTFL(MSG_DEBUG, "PriorityPacketSelector: current priority sum: %.2f (original: %.2f)", curpriosum, prioSum);

	// equalize sum of priorities
	double min, max;
	for (list<HostData*>::iterator iter=hosts.begin(); iter!=hosts.end(); iter++) {
		double prio = (*iter)->priority + (prioSum-curpriosum)/hostCount;
		(*iter)->priority = prio;

		if (iter==hosts.begin() || min>prio)
			min = prio;
		if (iter==hosts.begin() || max<prio)
			max = prio;
	}
	msg(MSG_DEBUG, "PriorityPacketSelector: minimum priority %.2f, maximum priority %.2f", min, max);
}


bool smallerpriothan(HostData* i, HostData* j)
{
	return i->priority<j->priority;
}


void PriorityPacketSelector::assignHosts2IDS()
{
	msg(MSG_DEBUG, "PriorityPacketSelector: reassigning hosts to IDSs");
	// sort hosts
	vector<int64_t> idsoctets;
	for (uint32_t i=0; i<numberOfQueues; i++) {
		ids[i].hosts.sort(smallerpriothan);
		uint64_t octets = 0;
		for (list<HostData*>::iterator liter = ids[i].hosts.begin(); liter!=ids[i].hosts.end(); liter++) {
			octets += (*liter)->nextTraffic;
		}
		idsoctets.push_back(octets);
	}
	restHosts.sort(greaterpriothan);
	msg(MSG_ERROR, "resthosts first: %.2f, last: %.2f", restHosts.front()->priority, restHosts.back()->priority);

	struct timeval curtime;
	gettimeofday(&curtime, 0);

	uint32_t resthostcount = restHosts.size();
	for (uint32_t i=0; i<numberOfQueues; i++) {
		uint32_t rcount = 0;
		uint32_t acount = 0;

		ids[i].hosts.sort(smallerpriothan);
		// remove hosts from IDS because too much traffic would be analyzed
		list<HostData*>::iterator iter = ids[i].hosts.begin();
		while (idsoctets[i]>(int64_t)ids[i].maxOctets && iter!=ids[i].hosts.end()) {
			HostData* host = *iter;
			if (!host->belowMinMonTime(curtime, minMonTime)) {
				iter = ids[i].hosts.erase(iter);
				ids[i].hostcount--;
				restHosts.push_back(host);
				idsoctets[i] -= host->nextTraffic;
				rcount++;
			} else {
				iter++;
			}
		}

		// reassign hosts
		iter = ids[i].hosts.begin();
		list<HostData*>::iterator riter = restHosts.begin();
		while (riter!=restHosts.end()) {
			PPDPRINTFL(MSG_DEBUG, "restHosts=%u", resthostcount);
			HostData* resthost = *riter;
			PPDPRINTFL(MSG_DEBUG, "PriorityPacketSelector: trying to move host %s (traffic %llu bytes) to IDS %d", IPToString(ntohl(resthost->ip)).c_str(), resthost->nextTraffic, i);

			// try to find hosts to remove from IDS
			list<HostData*> removedhosts;
			uint64_t remoctets = 0;
			if (iter!=ids[i].hosts.end()) {
				HostData* idshost = *iter;
				while (iter != ids[i].hosts.end() &&
						ids[i].maxOctets-idsoctets[i]<resthost->nextTraffic &&
						idshost->priority<resthost->priority) {
					if (!idshost->belowMinMonTime(curtime, minMonTime)) {
						PPDPRINTFL(MSG_DEBUG, "PriorityPacketSelector:temporarily removing host %s from IDS %d", IPToString(ntohl(idshost->ip)).c_str(), i);
						removedhosts.push_back(*iter);
						iter = ids[i].hosts.erase(iter);
						ids[i].hostcount--;
						idsoctets[i] -= idshost->nextTraffic;
						remoctets += idshost->nextTraffic;
					} else {
						iter++;
					}
					idshost = *iter;
				}
			}
			if ((int64_t)ids[i].maxOctets-idsoctets[i]>=(int64_t)resthost->nextTraffic) {
				// success! insert our host
				PPDPRINTFL(MSG_DEBUG, "PriorityPacketSelector:success, really inserting host %s to IDS %d", IPToString(ntohl(resthost->ip)).c_str(), i);
				ids[i].hosts.push_back(resthost);
				resthost->startMon = curtime;
				ids[i].hostcount++;
				idsoctets[i] += resthost->nextTraffic;
				riter = restHosts.erase(riter);
				resthostcount--;
				resthostcount += removedhosts.size();
				rcount += removedhosts.size();
				restHosts.insert(restHosts.end(), removedhosts.begin(), removedhosts.end());
				acount++;
			} else {
				if (ids[i].hosts.empty()) {
					// all hosts were removed from IDS, and still the new host cannot be added ... do it anyway
					PPDPRINTFL(MSG_DEBUG, "PriorityPacketSelector:success, inserting host %s to IDS %d, although it's got too much traffic", IPToString(ntohl(resthost->ip)).c_str(), i);
					ids[i].hosts.push_back(resthost);
					resthost->startMon = curtime;
					ids[i].hostcount++;
					idsoctets[i] += resthost->nextTraffic;
					riter = restHosts.erase(riter);
					resthostcount--;
					resthostcount += removedhosts.size();
					rcount += removedhosts.size();
					restHosts.insert(restHosts.end(), removedhosts.begin(), removedhosts.end());
					acount++;
				} else {
					// failure - go to next host
					PPDPRINTFL(MSG_DEBUG, "PriorityPacketSelector: failure, on to next host", IPToString(ntohl(resthost->ip)).c_str(), i);
					ids[i].hosts.insert(ids[i].hosts.begin(), removedhosts.begin(), removedhosts.end());
					ids[i].hostcount += removedhosts.size();
					idsoctets[i] += remoctets;
#ifdef DEBUG
					for (list<HostData*>::iterator miter = ids[i].hosts.begin(); miter!=ids[i].hosts.end(); miter++) {
						PPDPRINTFL(MSG_VDEBUG, "PriorityPacketSelector: IDS %d: %s", i, IPToString(ntohl((*miter)->ip)).c_str());
					}
					for (list<HostData*>::iterator miter = restHosts.begin(); miter!=restHosts.end(); miter++) {
						PPDPRINTFL(MSG_VDEBUG, "PriorityPacketSelector: restHosts: %s", IPToString(ntohl((*miter)->ip)).c_str());
					}
#endif
				}
			}
			riter++;
		}

		msg(MSG_DEBUG, "PriorityPacketSelector: IDS %u, hosts %u (+%u,-%u), est.load %llu, act.load %llu, max load %llu, dropped %llu",
				i, ids[i].hosts.size(), acount, rcount, idsoctets[i], ids[i].curForwOct, ids[i].maxOctets, ids[i].curDropOct);
	}

	msg(MSG_DEBUG, "PriorityPacketSelector: unmonitored hosts: %u", resthostcount);

	uint64_t docts = discardOctets;
	discardOctets -= docts;
	msg(MSG_DEBUG, "PriorityPacketSelector: discarded %llu bytes", docts);
}


void PriorityPacketSelector::setIpConfig()
{
	map<uint32_t, int> scfg;

	for (uint32_t i=0; i<numberOfQueues; i++) {
		list<HostData*>::iterator iter = ids[i].hosts.begin();
		for (list<HostData*>::iterator iter = ids[i].hosts.begin(); iter!=ids[i].hosts.end(); iter++) {
			scfg[htonl((*iter)->ip)] = i;
		}
	}

	ipSelector.initializeConfig(scfg, scfg);
}


void PriorityPacketSelector::updateData(list<IDSLoadStatistics>& lstats)
{
	list<IDSLoadStatistics>::iterator iter = lstats.begin();
	// ProcessStatisticsProvider only provides the sum of dropped octets/packets, get actual value of last interval
	for (uint32_t i=0; i<numberOfQueues; i++) {
		ids[i].curDropOct = iter->droppedOctets-ids[i].lastDropOct;
		ids[i].lastDropOct = iter->droppedOctets;
		ids[i].curDropPkt = iter->droppedPackets-ids[i].lastDropPkt;
		ids[i].lastDropPkt = iter->droppedPackets;
		ids[i].curForwOct = iter->forwardedOctets-ids[i].lastForwOct;
		ids[i].lastForwOct = iter->forwardedOctets;
		ids[i].curForwPkt = iter->forwardedPackets-ids[i].lastForwPkt;
		ids[i].lastForwPkt = iter->forwardedPackets;
		iter++;
	}

	updateIDSMaxRate();
	updateTrafficEstimation();
	updatePriorities();
	assignHosts2IDS();
	setIpConfig();
}


void PriorityPacketSelector::start()
{
	gettimeofday(&startTime, 0);
}

void PriorityPacketSelector::stop()
{
	startTime.tv_sec = 0;
	startTime.tv_usec = 0;
}

void PriorityPacketSelector::setUpdateInterval(uint32_t ms)
{
	updateInterval = ms;
}
