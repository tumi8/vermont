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


const uint32_t PriorityPacketSelector::WARN_HOSTCOUNT = 1<<22;



/**
 * WARNING: does not set hostcount by itself, it is set by PriorityPacketSelector
 */
PriorityNetConfig::PriorityNetConfig(uint32_t subnet, uint32_t mask, uint8_t maskbits, float weight)
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
PriorityPacketSelector::PriorityPacketSelector(list<PriorityNetConfig>& pnc, float startprio, struct timeval minmontime)
	: BasePacketSelector("PriorityPacketSelector"),
	  config(pnc),
	  startPrio(startprio),
	  hostCount(0),
	  minMonTime(minmontime),
	  discardOctets(0)
{
	msg(MSG_INFO, "PriorityPacketSelector: Creating host priority entries ...");

	// create host entries
	for (list<PriorityNetConfig>::iterator citer = config.begin(); citer!=config.end(); citer++) {
		citer->hostcount = insertSubnet(citer->subnet, citer->maskbits, citer->weight);
	}
	msg(MSG_INFO, "PriorityPacketSelector: Created %u entries", hostCount);

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
uint32_t PriorityPacketSelector::insertSubnet(uint32_t subnet, uint8_t maskbits, float weight)
{
	// iterate through all possible IP values
	bool warningdisp = false;
	for (uint32_t i=1; i<(1u<<(32-maskbits))-1; i++) {
		uint32_t ip = i | subnet;
		if (ip2host.find(ip)==ip2host.end()) {
			HostData* hd = new HostData(ip, startPrio, weight);
			hosts.push_back(hd);
			ip2host[ip] = hd;
			restHosts.push_back(hd);
			hostCount++;
			if (hostCount>WARN_HOSTCOUNT && !warningdisp) {
				msg(MSG_DIALOG, "PriorityPacketSelector: Warning, number of prioritized hosts is greated than %u!", WARN_HOSTCOUNT);
				warningdisp = true;
			}
		}
	}
	return (2u<<(32-maskbits))-2;
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
	float maxw = ascw.back()->weight;
	float minwsum = 0; // M_minw(r,m)^T/W (temporary value)
	for (list<PriorityNetConfig*>::iterator iter = ascw.begin(); iter!=ascw.end(); iter++) {
		if (maxw<=((float)hostCount/(minwsum+(*iter)->hostcount/(*iter)->weight))) {
			// portion of the hosts in this subnet is needed to fulfil criteria
			maxHostPrioChange += (uint32_t)((*iter)->weight*((float)hostCount/maxw)-minwsum);
			break;
		} else {
			// maxw is not reached yet, add hosts to temporary value
			minwsum += (*iter)->hostcount/(*iter)->weight;
			maxHostPrioChange += (*iter)->hostcount;
		}
	}
	msg(MSG_INFO, "PriorityPacketSelector: maxHostPrioChange=%u", maxHostPrioChange);
}


int PriorityPacketSelector::decide(Packet *p)
{
	int qid = ipSelector.decide(p);
	if (qid>=0)	ids[qid].curOctets += p->data_length;
	else discardOctets += p->data_length;
	return qid;
}

void PriorityPacketSelector::updateIDSMaxRate(list<IDSLoadStatistics>& lstats)
{
	for (uint32_t i=0; i<numberOfQueues; i++) {

	}
}


void PriorityPacketSelector::updateTrafficEstimation()
{
	msg(MSG_DEBUG, "PriorityPacketSelector: updating host traffic estimation");
	HostStatisticsGenerator* hoststats = HostStatisticsGeneratorFactory::getInstance();
	for (map<uint32_t, HostData*>::iterator iter = ip2host.begin(); iter!=ip2host.end(); iter++) {
		uint64_t octets;
		if (hoststats->getOctets(iter->first, octets)) {
			iter->second->lastTraffic = octets;
			if (octets==0) iter->second->nextTraffic = hoststats->getZeroOctets();
			else iter->second->nextTraffic = octets;
		} else {
			// we do not have any new data for this host ...
			iter->second->lastTraffic = 0;
			iter->second->nextTraffic = hoststats->getZeroOctets();
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
	for (list<HostData*>::iterator iter = hosts.begin(); iter!=hosts.end(); iter++) {
		HostData* hd = *iter;
		sortedhosts.push_back(hd);
	}
	sort(sortedhosts.begin(), sortedhosts.end(), greaterpriothan);

	// update priorities of monitored hosts
	float curpriosum = 0;
	uint32_t count = 0;
	for (vector<HostData*>::iterator iter=sortedhosts.begin(); iter!=sortedhosts.end(); iter++) {
		if (count<=maxHostPrioChange) {
			(*iter)->priority -= 1.0/(*iter)->weight;
		}
		curpriosum += (*iter)->priority;
		count++;
	}

	// equalize sum of priorities
	for (map<uint32_t, HostData*>::iterator iter=ip2host.begin(); iter!=ip2host.end(); iter++) {
		iter->second->priority += (prioSum-curpriosum)/hostCount;
	}
}

bool smallerpriothan(HostData* i, HostData* j)
{
	return i->priority<j->priority;
}


void PriorityPacketSelector::assignHosts2IDS()
{
	msg(MSG_DEBUG, "PriorityPacketSelector: reassigning hosts to IDSs");
	// sort hosts
	vector<uint64_t> idsoctets;
	for (uint32_t i=0; i<numberOfQueues; i++) {
		ids[i].hosts.sort(smallerpriothan);
		uint64_t octets = 0;
		for (list<HostData*>::iterator liter = ids[i].hosts.begin(); liter!=ids[i].hosts.end(); liter++) {
			octets += (*liter)->nextTraffic;
		}
		idsoctets.push_back(octets);
	}

	struct timeval curtime;
	gettimeofday(&curtime, 0);
	// FIXME: this is slow. Speed it up!1!!
	for (uint32_t i=0; i<numberOfQueues; i++) {
		uint32_t rcount = 0;
		uint32_t acount = 0;

		// remove hosts from IDS because too much traffic would be analyzed
		list<HostData*>::iterator iter = ids[i].hosts.begin();
		while (idsoctets[i]>ids[i].maxOctets && iter!=ids[i].hosts.end()) {
			HostData* host = *iter;
			if (!host->belowMinMonTime(curtime, minMonTime)) {
				iter = ids[i].hosts.erase(iter);
				ids[i].hostcount--;
				restHosts.push_back(*iter);
				idsoctets[i] -= host->nextTraffic;
				rcount++;
			} else {
				iter++;
			}
		}

		// reassign hosts
		iter = ids[i].hosts.begin();
		uint32_t resthostcount = restHosts.size();
		while (!restHosts.empty()) {
			DPRINTFL(MSG_VDEBUG, "restHosts=%u", resthostcount);
			HostData* resthost = restHosts.front();

			// try to find hosts to remove from IDS
			list<HostData*> removedhosts;
			uint64_t tmpoctets = idsoctets[i];
			if (iter!=ids[i].hosts.end()) {
				HostData* idshost = *iter;
				while (ids[i].maxOctets-idsoctets[i]<resthost->nextTraffic &&
						idshost->priority<resthost->priority) {
					if (!idshost->belowMinMonTime(curtime, minMonTime)) {
						iter = ids[i].hosts.erase(iter);
						ids[i].hostcount--;
						removedhosts.push_back(*iter);
						tmpoctets -= idshost->nextTraffic;
					} else {
						iter++;
						idshost = *iter;
					}
				}
			}
			if (ids[i].maxOctets-idsoctets[i]>=restHosts.front()->nextTraffic) {
				// success! insert our host
				ids[i].hosts.push_back(restHosts.front());
				ids[i].hostcount++;
				tmpoctets += resthost->nextTraffic;
				restHosts.erase(restHosts.begin());
				resthostcount--;
				rcount += removedhosts.size();
				restHosts.insert(restHosts.end(), removedhosts.begin(), removedhosts.end());
				idsoctets[i] = tmpoctets;
				acount++;
			} else {
				// failure - go to next IDS
				ids[i].hosts.insert(ids[i].hosts.begin(), removedhosts.begin(), removedhosts.end());

				break;
			}
		}

		uint64_t octs = ids[i].curOctets;
		ids[i].curOctets -= octs;
		msg(MSG_DEBUG, "PriorityPacketSelector: IDS %u, hosts %u (+%u,-%u), est.load %llu, act.load %llu, max load %llu",
				i, ids[i].hosts.size(), acount, rcount, idsoctets[i], octs, ids[i].maxOctets);
	}

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
	updateIDSMaxRate(lstats);
	updateTrafficEstimation();
	updatePriorities();
	assignHosts2IDS();
	setIpConfig();
}
