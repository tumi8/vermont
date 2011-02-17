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

const uint32_t PriorityPacketSelector::WARN_HOSTCOUNT = 1 << 22;

InstanceManager<IpfixDataRecord> PriorityPacketSelector::dataRecordIM("PriorityPacketSelector", 0);
InstanceManager<IpfixTemplateRecord> PriorityPacketSelector::templateRecordIM("PriorityPacketSelector", 0);

//#define PPDPRINTFL(lvl, fmt, args...)  msg2(__LINE__, __FILE__, __PRETTY_FUNCTION__, __func__, lvl, fmt, ##args)
#define PPDPRINTFL(lvl, fmt, args...)

HostData::HostData(uint32_t ip, double p, double w) :
	priority(p), oldpriority(p), weight(w), ip(ip), assignedIdsId(-1), nextTraffic(0), lastTraffic(
			0)
{
	startMon.tv_sec = 0;
	startMon.tv_usec = 0;
}

/**
 * WARNING: does not set hostcount by itself, it is set by PriorityPacketSelector
 */
PriorityNetConfig::PriorityNetConfig(uint32_t subnet, uint32_t mask, uint8_t maskbits,
		double weight) :
	subnet(subnet), mask(mask), maskbits(maskbits), hostcount(0), weight(weight)
{
}

WeightModifierConfig::WeightModifierConfig(float quantile, float weight)
	: quantile(quantile),
	  weight(weight)
{
}

bool HostData::belowMinMonTime(struct timeval& tv, struct timeval& minmontime)
{
	struct timeval diff;
	timeval_subtract(&diff, &tv, &startMon);
	return compareTime(minmontime, diff) > 0;
}

/**
 * @param pnc expected in order of decreasing bits in the mask!
 */
PriorityPacketSelector::PriorityPacketSelector(list<PriorityNetConfig>& pnc, double startprio,
		struct timeval minmontime, uint32_t maxspeed, list<WeightModifierConfig>& wmc)
	: BasePacketSelector("PriorityPacketSelector"),
	  config(pnc),
	  weightModifiers(wmc),
	  hostCount(0),
	  startPrio(startprio),
	  minMonTime(minmontime),
	  discardOctets(0),
	  updateInterval(0),
	  packetHostInfo(0),
	  newPacketHostInfo(0),
	  maxSpeed(maxspeed),
	  round(0),
	  flowExporter(0),
	  sourceId(new IpfixRecord::SourceID),
	  templateInfo(new TemplateInfo)
{
	msg(MSG_INFO, "PriorityPacketSelector: Creating host priority entries ...");

	startMaxSpeed = 500000;

	// create host entries
	for (list<PriorityNetConfig>::iterator citer = config.begin(); citer != config.end(); citer++) {
		citer->hostcount = insertSubnet(citer->subnet, citer->maskbits, citer->weight);
	}
	prioSum = startprio * hostCount;
	msg(MSG_INFO, "PriorityPacketSelector: Created %u entries with start priority %.2f", hostCount, startprio);

	startTime.tv_sec = 0;
	startTime.tv_usec = 0;

	calcMaxHostPrioChange();

	list<uint32_t> subnets;
	list<uint32_t> masks;
	for (list<PriorityNetConfig>::iterator iter = config.begin(); iter != config.end(); iter++) {
		subnets.push_back(htonl(iter->subnet));
		masks.push_back(htonl(iter->mask));
	}
	ipSelector.setSubnets(subnets, masks);

	float lastquantile = 0;
	for (list<WeightModifierConfig>::iterator iter = weightModifiers.begin(); iter!=weightModifiers.end(); iter++) {
		if (lastquantile >= iter->quantile)
			THROWEXCEPTION("PriorityPacketSelector: traffic quantiles given in weight modifier configuration are not monotonically increasing!");
		lastquantile = iter->quantile;
	}
	if (weightModifiers.size()>0 && lastquantile!=1)
		THROWEXCEPTION("PriorityPacketSelector: last traffic quantile given in weight modifier configuration is not equal to 1!");

	roundStart.tv_sec = 0;
	roundStart.tv_usec = 0;

	// TODO: make this configurable!
	sourceId->observationDomainId = 999;

	// build template when statistics flows need to be sent
	InformationElement::IeInfo recordtypes[] = {
			InformationElement::IeInfo(IPFIX_TYPEID_sourceIPv4Address, 0),
			InformationElement::IeInfo(IPFIX_ETYPEID_monitored, IPFIX_PEN_vermont),
			InformationElement::IeInfo(IPFIX_TYPEID_octetDeltaCount, 0),
			InformationElement::IeInfo(IPFIX_TYPEID_flowStartMilliSeconds, 0),
			InformationElement::IeInfo(IPFIX_TYPEID_flowEndMilliSeconds, 0)
	};

	templateInfo->templateId = 9999;
	templateInfo->setId = TemplateInfo::IpfixTemplate;
	templateInfo->fieldCount = ARRAY_SIZE(recordtypes);
	templateInfo->fieldInfo = new TemplateInfo::FieldInfo[ARRAY_SIZE(recordtypes)];

	uint16_t offset = 0;
	for (uint32_t i=0; i<ARRAY_SIZE(recordtypes); i++) {
		const struct ipfix_identifier* ident = ipfix_id_lookup(recordtypes[i].id, recordtypes[i].enterprise);
		templateInfo->fieldInfo[i].type = recordtypes[i];
		templateInfo->fieldInfo[i].type.length = ident->length;
		templateInfo->fieldInfo[i].offset = offset;
		offset += ident->length;
		templateInfo->fieldInfo[i].privDataOffset = 0;
		templateInfo->fieldInfo[i].isVariableLength = false;
	}
	msg(MSG_INFO, "PriorityPacketSelector: Built IpfixDataRecord with %u elements", ARRAY_SIZE(recordtypes));
}

PriorityPacketSelector::~PriorityPacketSelector()
{
	while (!hosts.empty()) {
		delete hosts.front();
		hosts.pop_front();
	}

	if (packetHostInfo) {
		delete packetHostInfo[0]->selectorData;
		for (uint32_t i = 0; i < numberOfQueues; i++)
			delete packetHostInfo[i];
		delete[] packetHostInfo;
	}
	if (newPacketHostInfo) {
		delete newPacketHostInfo[0]->selectorData;
		for (uint32_t i = 0; i < numberOfQueues; i++)
			delete newPacketHostInfo[i];
		delete[] newPacketHostInfo;
	}
}

void PriorityPacketSelector::setQueueCount(uint32_t n)
{
	BasePacketSelector::setQueueCount(n);
	for (uint32_t i = 0; i < numberOfQueues; i++) {
		const double KP = 0.01;
		ids.push_back(IDSData(startMaxSpeed, KP));
	}

	if (packetHostInfo)
		delete[] packetHostInfo;
	packetHostInfo = new volatile PacketHostInfo*[n];
	for (uint32_t i = 0; i < n; i++)
		packetHostInfo[i] = 0;
}

/**
 * @returns number of generated host entries
 */
uint32_t PriorityPacketSelector::insertSubnet(uint32_t subnet, uint8_t maskbits, double weight)
{
	if (maskbits == 32) {
		// insert single host
		if (ip2host.find(subnet) == ip2host.end()) {
			HostData* hd = new HostData(subnet, startPrio, weight);
			hosts.push_back(hd);
			ip2host[subnet] = hd;
			restHosts.push_back(hd);
			hostCount++;
		}
		return 1;

	}
	else {
		// iterate through all possible IP values
		bool warningdisp = false;
		uint32_t hcount = 0;
		for (uint32_t i = 1; i < (1u << (32 - maskbits)) - 1; i++) {
			uint32_t ip = i | subnet;
			if (ip2host.find(ip) == ip2host.end()) {
				HostData* hd = new HostData(ip, drand48(), weight);
				hosts.push_back(hd);
				ip2host[ip] = hd;
				restHosts.push_back(hd);
				hostCount++;
				hcount++;
				if (hostCount > WARN_HOSTCOUNT && !warningdisp) {
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
	for (list<PriorityNetConfig>::iterator citer = config.begin(); citer != config.end(); citer++) {
		temp.push_back(&(*citer));
	}
	while (!temp.empty()) {
		list<PriorityNetConfig*>::iterator maxw = temp.begin();
		for (list<PriorityNetConfig*>::iterator iter = ++temp.begin(); iter != temp.end(); iter++) {
			if ((*maxw)->weight > (*iter)->weight) {
				maxw = iter;
			}
		}
		ascw.push_back(*maxw);
		temp.erase(maxw);
	}

	maxHostPrioChange = 0;
	double invmaxw = 1.0 / ascw.back()->weight;
	double minwsum = 0; // M_minw(r,m)^T/W (temporary value)
	for (list<PriorityNetConfig*>::iterator iter = ascw.begin(); iter != ascw.end(); iter++) {
		if (invmaxw <= ((double) hostCount / (minwsum + (*iter)->hostcount / (*iter)->weight))) {
			// portion of the hosts in this subnet is needed to fulfil criteria
			maxHostPrioChange += (uint32_t) ((*iter)->weight * ((double) hostCount * invmaxw)
					- minwsum);
			break;
		}
		else {
			// invmaxw is not reached yet, add hosts to temporary value
			minwsum += (*iter)->hostcount / (*iter)->weight;
			maxHostPrioChange += (*iter)->hostcount;
		}
	}
	msg(MSG_INFO, "PriorityPacketSelector: maxHostPrioChange=%u", maxHostPrioChange);
	if (maxHostPrioChange == 0) {
		THROWEXCEPTION("PriorityPacketSelector: due to the maximum weight assigned to at least one host, this priority system does not work. See section 'extreme cases' in paper.");
	}
}

int PriorityPacketSelector::decide(Packet *p)
{
	// apply new host information
	if (newPacketHostInfo && packetHostInfo) {
		if (packetHostInfo[0] && packetHostInfo[0]->selectorData)
			delete packetHostInfo[0]->selectorData;
		for (uint32_t i = 0; i < numberOfQueues; i++)
			if (packetHostInfo[i])
				delete packetHostInfo[i];
		delete[] packetHostInfo;
		packetHostInfo = newPacketHostInfo;
		newPacketHostInfo = 0;
	}

	if (p->customData)
		THROWEXCEPTION("PriorityPacketSelector: received packet already contains custom data - this must not happen. Check the configuration!");

	// do not forward a packet until the first host list has been generated
	if (!packetHostInfo[0])
		return -1;

	p->customData = packetHostInfo;

	int qid = ipSelector.decide(p);

	if (qid < 0)
		discardOctets += p->data_length;
	else
		p->customData = const_cast<PacketHostInfo*>(packetHostInfo[qid]);

	return qid;
}

void PriorityPacketSelector::updateIDSMaxRate()
{
	float slowstartratio = 1.2;
	float normalratio = 1.01;
	uint64_t slowstartrounds=200;

	// if we just started the process, do nothing
	struct timeval curtime;
	struct timeval diff;
	gettimeofday(&curtime, 0);
	timeval_subtract(&diff, &curtime, &startTime);
	if (diff.tv_sec < PPS_DEFAULT_MINSTARTDELAY)
		return;

	for (uint32_t i = 0; i < numberOfQueues; i++) {
		uint64_t oldmax = ids[i].maxOctets;
		if (maxSpeed>0) {
			ids[i].maxOctets = maxSpeed;
		} else {
			uint64_t droppedoct = ids[i].curDropOct + ids[i].controlDropOct;

			float ratio = normalratio;
			if (round<slowstartrounds) ratio = slowstartratio;

			if (droppedoct > 0) {
				if (ids[i].curForwOct < ids[i].maxOctets
					&& ids[i].maxOctets>startMaxSpeed)
					ids[i].maxOctets = (float) ids[i].maxOctets / ratio;
			}
			else if (ids[i].curForwOct > ids[i].maxOctets / 2
					&& (ids[i].expOctets>0.75*ids[i].maxOctets)) // the IDS should have been almost completely filled by the assignment process
			{
				ids[i].maxOctets = (float) ids[i].maxOctets * ratio;
			}
		}
		msg(MSG_INFO, "PriorityPacketSelector: IDS %u maximum octets - old: %llu, new: %llu", i, oldmax, ids[i].maxOctets);
	}
}

void PriorityPacketSelector::updateTrafficEstimation()
{
	msg(MSG_INFO, "PriorityPacketSelector: updating host traffic estimation");
	HostStatisticsGenerator* hoststats = HostStatisticsGeneratorFactory::getInstance();
	uint32_t hsglength = hoststats->getIntervalLength();
	double ratio = (double) updateInterval / hsglength;
	uint64_t zerooctets = ratio * hoststats->getZeroOctets();
	for (list<HostData*>::iterator iter = hosts.begin(); iter != hosts.end(); iter++) {
		HostData* host = *iter;
		uint64_t octets;
		if (hoststats->getOctets(host->ip, octets)) {
			octets *= ratio;
			host->lastTraffic = octets;
			if (octets == 0)
				host->nextTraffic = zerooctets;
			else
				host->nextTraffic = octets;
		}
		else {
			// we do not have any new data for this host ...
			host->lastTraffic = 0;
			host->nextTraffic = zerooctets;
		}
	}
}

bool greaterpriothan(HostData* i, HostData* j)
{
	return i->priority > j->priority;
}

float PriorityPacketSelector::getWeightModifier(HostStatisticsGenerator* hoststats, HostData* host)
{
	if (weightModifiers.size()==0) return 1;

	float quantile;
	float modifier = 1;
	if (!hoststats->getTrafficQuantile(host->ip, quantile))
		quantile = 0; // we assume that when this host was not measured yet, it will have traffic index=0, as it did not transfer any data

	for (list<WeightModifierConfig>::iterator iter = weightModifiers.begin(); iter!=weightModifiers.end(); iter++) {
		if (iter->quantile<quantile) {
			modifier *= iter->weight;
			break;
		}
	}
	return modifier;
}

void PriorityPacketSelector::updatePriorities()
{
	msg(MSG_INFO, "PriorityPacketSelector: updating host priorities");

	finishedHosts.sort(greaterpriothan);

	// update priorities of successfully monitored hosts
	uint32_t count = 0;
	struct timeval curtime;
	gettimeofday(&curtime, 0);
	msg(MSG_INFO, "PriorityPacketSelector: decreased priority of %u hosts", finishedHosts.size());
	HostStatisticsGenerator* hoststats = HostStatisticsGeneratorFactory::getInstance();
	for (list<HostData*>::iterator iter = finishedHosts.begin(); iter != finishedHosts.end(); iter++) {
		if (round>0 && count <= maxHostPrioChange) {
			PPDPRINTFL(MSG_VDEBUG, "PriorityPacketSelector: old priority of monitored host %s: %.2f", IPToString(htonl((*iter)->ip)).c_str(), (double)(*iter)->priority);

			(*iter)->priority -= 1.0 / ((*iter)->weight * getWeightModifier(hoststats, *iter));
			count++;
		}
	}
	finishedHosts.clear();

	// build sum of priorities
	double curpriosum = 0;
	for (uint32_t i = 0; i < numberOfQueues; i++) {
		for (list<HostData*>::iterator iter = ids[i].hosts.begin(); iter != ids[i].hosts.end(); iter++) {
			HostData* hd = *iter;
			curpriosum += hd->priority;
		}
	}
	for (list<HostData*>::iterator iter = restHosts.begin(); iter != restHosts.end(); iter++) {
		curpriosum += (*iter)->priority;
		PPDPRINTFL(MSG_VDEBUG, "PriorityPacketSelector: old priority of non-monitored host %s: %.2f", IPToString(htonl((*iter)->ip)).c_str(), (double)(*iter)->priority);
	} PPDPRINTFL(MSG_INFO, "PriorityPacketSelector: current priority sum: %.2f (original: %.2f)", curpriosum, prioSum);

	// equalize sum of priorities
	double min, max;
	double diff = (prioSum - curpriosum) / hostCount;
	for (list<HostData*>::iterator iter = hosts.begin(); iter != hosts.end(); iter++) {
		double prio = (*iter)->priority + diff;
		(*iter)->priority = prio;

		if (iter == hosts.begin() || min > prio)
			min = prio;
		if (iter == hosts.begin() || max < prio)
			max = prio;
	}
	msg(MSG_INFO, "PriorityPacketSelector: minimum priority %.2f, maximum priority %.2f, diff %.2f", min, max, diff);
}

bool smallerpriothan(HostData* i, HostData* j)
{
	return i->priority < j->priority;
}

void PriorityPacketSelector::sendFlowRecord(HostData* host, bool monitored, uint64_t starttime, uint64_t endtime)
{
	if (!flowExporter) return;

	IpfixDataRecord* record = dataRecordIM.getNewInstance();
	record->sourceID = sourceId;
	record->templateInfo = templateInfo;
	record->dataLength = sizeof(IpfixData);
	boost::shared_array<IpfixRecord::Data> data = boost::shared_array<IpfixRecord::Data>(new IpfixRecord::Data[sizeof(IpfixData)]);
	record->message = data;
	record->data = data.get();
	IpfixData* idata = reinterpret_cast<IpfixData*>(data.get());
	idata->ip = htonl(host->ip);
	idata->monitored = monitored;
	idata->octets = htonll(host->lastTraffic);
	idata->flowStartTime = htonll(starttime);
	idata->flowEndTime = htonll(endtime);
	flowExporter->receive(record);
}

void PriorityPacketSelector::assignHosts2IDS()
{
	msg(MSG_INFO, "PriorityPacketSelector: reassigning hosts to IDSs");

	// sort hosts
	vector<int64_t> idsoctets;
	for (uint32_t i = 0; i < numberOfQueues; i++) {
		ids[i].hosts.sort(smallerpriothan);
		uint64_t octets = 0;
		for (list<HostData*>::iterator liter = ids[i].hosts.begin(); liter != ids[i].hosts.end(); liter++) {
			octets += (*liter)->nextTraffic;
		}
		idsoctets.push_back(octets * ids[i].estRatio); // estRatio comes from the control loop that tries to adjust to estimation errors
	}
	restHosts.sort(greaterpriothan);
	msg(MSG_ERROR, "resthosts first: %.2f (%s/%llu bytes), last: %.2f (%s/%llu bytes)",
			restHosts.front()->priority, IPToString(restHosts.front()->ip).c_str(), restHosts.front()->lastTraffic,
			restHosts.back()->priority, IPToString(restHosts.back()->ip).c_str(), restHosts.back()->lastTraffic);

	// randomize list of IDS
	list<uint32_t> idxrand;
	vector<uint32_t> indexes(numberOfQueues);
	for (uint32_t i = 0; i < numberOfQueues; i++) {
		indexes[i] = i;
	}
	while (!indexes.empty()) {
		uint32_t j = (rand() % indexes.size());
		idxrand.push_back(indexes[j]);
		indexes.erase(indexes.begin() + j);
	}

	struct timeval curtime;
	gettimeofday(&curtime, 0);
	uint32_t resthostcount = restHosts.size();
	uint64_t roundstart = (uint64_t)roundStart.tv_sec*1000+roundStart.tv_usec/1000;
	uint64_t roundend = (uint64_t)curtime.tv_sec*1000+curtime.tv_usec/1000;
	// iterate through randomized list
	for (list<uint32_t>::iterator idxiter = idxrand.begin(); idxiter != idxrand.end(); idxiter++) {
		uint32_t i = *idxiter;
		double estratio = ids[i].estRatio;
		uint32_t rcount = 0;
		uint32_t acount = 0;
		uint64_t maxoctets = ids[i].maxOctets;

		ids[i].hosts.sort(smallerpriothan);

		// remove forcibly dropped hosts
		list<HostData*>::iterator hiter = ids[i].hosts.begin();
		uint32_t fdhosts = 0;
		while (hiter!=ids[i].hosts.end()) {
			HostData* host = *hiter;
			if (wasHostDropped(host)) {
				sendFlowRecord(host, !host->belowMinMonTime(curtime, minMonTime), roundstart, roundend);
				hiter = ids[i].hosts.erase(hiter);
				ids[i].hostcount--;
				restHosts.push_back(host);
				resthostcount++;
				fdhosts++;
			} else {
				hiter++;
			}
		}
		rcount += fdhosts;

		// remove all hosts that have been monitored for the minimum monitoring interval
		uint32_t succhosts = 0;
		hiter = ids[i].hosts.begin();
		while (hiter!=ids[i].hosts.end()) {
			HostData* host = *hiter;
			if (!host->belowMinMonTime(curtime, minMonTime)) {
				sendFlowRecord(host, !host->belowMinMonTime(curtime, minMonTime), roundstart, roundend);
				hiter = ids[i].hosts.erase(hiter);
				ids[i].hostcount--;
				restHosts.push_back(host);
				resthostcount++;
				succhosts++;
				finishedHosts.push_back(host);
			} else {
				hiter++;
			}
		}
		rcount += succhosts;

		// remove hosts from IDS because too much traffic would be analyzed
		list<HostData*>::iterator iter = ids[i].hosts.begin();
		uint32_t ohosts = 0;
		while (idsoctets[i] > (int64_t) maxoctets && iter != ids[i].hosts.end() && ids[i].hostcount>1) {
			HostData* host = *iter;
			iter = ids[i].hosts.erase(iter);
			ids[i].hostcount--;
			restHosts.push_back(host);
			resthostcount++;
			sendFlowRecord(host, !host->belowMinMonTime(curtime, minMonTime), roundstart, roundend);
			idsoctets[i] -= host->nextTraffic * estratio;
			rcount++;
			ohosts++;
		}
		msg(MSG_INFO, "PriorityPacketSelector: IDS %d: removed %u overload, %u forcibly dropped, %u successful hosts", i, ohosts, fdhosts, succhosts);

		// reassign hosts
		list<HostData*>::iterator riter = restHosts.begin();
		while (riter != restHosts.end()) {
			iter = ids[i].hosts.begin();
			PPDPRINTFL(MSG_INFO, "restHosts=%u", resthostcount);
			HostData* resthost = *riter;
			PPDPRINTFL(MSG_INFO, "PriorityPacketSelector: trying to move host %s (traffic %llu bytes) to IDS %d", IPToString(ntohl(resthost->ip)).c_str(), resthost->nextTraffic, i);

			// try to find hosts to remove from IDS
			list<HostData*> removedhosts;
			uint64_t remoctets = 0;

			HostData* idshost = *iter;
			while (iter != ids[i].hosts.end()
					&& maxoctets - idsoctets[i] < resthost->nextTraffic * estratio
					&& idshost->priority < resthost->priority) {
				PPDPRINTFL(MSG_INFO, "PriorityPacketSelector:temporarily removing host %s from IDS %d", IPToString(ntohl(idshost->ip)).c_str(), i);
				removedhosts.push_back(*iter);
				iter = ids[i].hosts.erase(iter);
				ids[i].hostcount--;
				idsoctets[i] -= idshost->nextTraffic * estratio;
				remoctets += idshost->nextTraffic * estratio;
				idshost = *iter;
			}

			if ((int64_t) maxoctets - idsoctets[i] >= (int64_t) resthost->nextTraffic * estratio) {
				// we have free data rate -- insert our host
				PPDPRINTFL(MSG_INFO, "PriorityPacketSelector:success, really inserting host %s to IDS %d", IPToString(ntohl(resthost->ip)).c_str(), i);
				ids[i].hosts.push_back(resthost);
				resthost->startMon = curtime;
				resthost->oldpriority = resthost->priority;
				ids[i].hostcount++;
				idsoctets[i] += resthost->nextTraffic * estratio;
				riter = restHosts.erase(riter);
				resthostcount--;
				resthostcount += removedhosts.size();
				rcount += removedhosts.size();
				restHosts.insert(restHosts.end(), removedhosts.begin(), removedhosts.end());
				for (list<HostData*>::iterator iter=removedhosts.begin(); iter!=removedhosts.end(); iter++)
					sendFlowRecord(*iter, true, roundstart, roundend);
				acount++;
			} else {
				if (ids[i].hosts.empty()) {
					// all hosts were removed from IDS, and still the new host cannot be added ... do it anyway
					PPDPRINTFL(MSG_INFO, "PriorityPacketSelector:success, inserting host %s to IDS %d, although it's got too much traffic", IPToString(ntohl(resthost->ip)).c_str(), i);
					ids[i].hosts.push_back(resthost);
					resthost->startMon = curtime;
					ids[i].hostcount++;
					idsoctets[i] += resthost->nextTraffic * estratio;
					riter = restHosts.erase(riter);
					resthostcount--;
					resthostcount += removedhosts.size();
					rcount += removedhosts.size();
					restHosts.insert(restHosts.end(), removedhosts.begin(), removedhosts.end());
					for (list<HostData*>::iterator iter=removedhosts.begin(); iter!=removedhosts.end(); iter++)
						sendFlowRecord(*iter, true, roundstart, roundend);
					acount++;
				} else {
					// failure - go to next host
					PPDPRINTFL(MSG_INFO, "PriorityPacketSelector: failure, on to next host", IPToString(ntohl(resthost->ip)).c_str(), i);
					ids[i].hosts.insert(ids[i].hosts.begin(), removedhosts.begin(),	removedhosts.end());
					ids[i].hostcount += removedhosts.size();
					idsoctets[i] += remoctets;
//#ifdef DEBUG

	/*				for (list<HostData*>::iterator miter = restHosts.begin(); miter!=restHosts.end(); miter++) {
						PPDPRINTFL(MSG_VDEBUG, "PriorityPacketSelector: restHosts: %s", IPToString(ntohl((*miter)->ip)).c_str());
					}*/
//#endif
				}
			}
			riter++;
		}

		ids[i].expOctets = (double) idsoctets[i];
		msg(MSG_INFO, "PriorityPacketSelector: IDS %u, hosts %u (+%u,-%u), est.load %llu, act.load %llu, max load %llu, contrdropped %llu, dropped %llu, max qu %u, cur qu %u",
				i, ids[i].hosts.size(), acount, rcount, ids[i].expOctets, ids[i].curForwOct, ids[i].maxOctets, ids[i].controlDropOct, ids[i].curDropOct, ids[i].maxQueueSize, ids[i].curQueueSize);
		HostData* host = *(ids[i].hosts.begin());
		if (!ids[i].hosts.empty()) msg(MSG_INFO, "PriorityPacketSelector: IDS %u, first IP %s", i, IPToString(host->ip).c_str());
	}

	msg(MSG_INFO, "PriorityPacketSelector: unmonitored hosts: %u", resthostcount);

	uint64_t docts = discardOctets;
	discardOctets -= docts;
	msg(MSG_INFO, "PriorityPacketSelector: discarded %llu bytes", docts);
}

void PriorityPacketSelector::setIpConfig()
{
	if (shutdownFlag) return;

	// wait until the hostdata structure has been processed
	if (newPacketHostInfo) {
		msg(MSG_INFO, "failed to set new host info - maybe no packet has arrived?");
	};

	HostHashtable* hh = new HostHashtable;

	// build tables for PCAPExporterMem and its fast removal
	volatile PacketHostInfo** philist = new volatile PacketHostInfo*[numberOfQueues];
	for (uint32_t idsidx = 0; idsidx < numberOfQueues; idsidx++) {
		PacketHostInfo* phi = new PacketHostInfo;
		phi->sortedHosts = ids[idsidx].hosts;
		phi->sortedHosts.sort(greaterpriothan);

		uint32_t hidx = 0;
		for (list<HostData*>::iterator iter = phi->sortedHosts.begin(); iter != phi->sortedHosts.end(); iter++) {
			SelectorHostData shd = SelectorHostData(idsidx);
			(*iter)->id = hidx++;
			shd.hostid = (*iter)->id;
			(*hh)[htonl((*iter)->ip)] = shd;
			(*iter)->assignedIdsId = idsidx;
		}

		phi->currentHost = 0;
		phi->hostCount = hidx;
		phi->selectorData = hh;
		phi->controlDropped = 0;
		phi->critQueueSize = queueSizes[idsidx]>>1;
		phi->maxHostId = hidx - 1;
		phi->minWarningShown = false;
		philist[idsidx] = phi;

		/*printf("IDS %d: \n", idsidx);
		for (list<HostData*>::iterator miter = phi->sortedHosts.begin(); miter!=phi->sortedHosts.end(); miter++) {
			printf("%s(%.2f) ", IPToString(ntohl((*miter)->ip)).c_str(), (*miter)->priority);
		}
		printf("\n");*/

	}
	newPacketHostInfo = philist;
}

void PriorityPacketSelector::updateEstRatio()
{
	// update control system for each IDS
	for (uint32_t i = 0; i < numberOfQueues; i++) {
		double w = ids[i].expOctets;
		if (w > 0) {
			double y = ids[i].curForwOct + ids[i].curDropOct + ids[i].controlDropOct;
			double e = w - y;
			double u = ids[i].kp * e;
			double d = -u / w;
			if (fabs(d) > 0.01)
				d = (d > 0 ? 0.01 : -0.01);
			ids[i].estRatio += d;
			if (ids[i].estRatio < 0)
				ids[i].estRatio -= d;
			msg(MSG_INFO, "PriorityPacketSelector: est. ratio for IDS %u = %.3f (+=%.3f), e=%.0f, u=%.3f", i, ids[i].estRatio, d, e, u);
		}
	}
}

void PriorityPacketSelector::updateData(struct timeval curtime, list<IDSLoadStatistics>& lstats)
{
	list<IDSLoadStatistics>::iterator iter = lstats.begin();
	// ProcessStatisticsProvider only provides the sum of dropped octets/packets, get actual value of last interval
	for (uint32_t i = 0; i < numberOfQueues; i++) {
		ids[i].curDropOct = iter->droppedOctets - ids[i].lastDropOct;
		ids[i].lastDropOct = iter->droppedOctets;
		ids[i].curDropPkt = iter->droppedPackets - ids[i].lastDropPkt;
		ids[i].lastDropPkt = iter->droppedPackets;
		ids[i].curForwOct = iter->forwardedOctets - ids[i].lastForwOct;
		ids[i].lastForwOct = iter->forwardedOctets;
		ids[i].curForwPkt = iter->forwardedPackets - ids[i].lastForwPkt;
		ids[i].lastForwPkt = iter->forwardedPackets;
		ids[i].curQueueSize = iter->curQueueSize;
		ids[i].maxQueueSize = iter->maxQueueSize;
		ids[i].controlDropOct = 0;
		if (packetHostInfo && packetHostInfo[i])
			ids[i].controlDropOct = packetHostInfo[i]->controlDropped;
		iter++;
	}

	updateEstRatio();
	updateIDSMaxRate();
	updateTrafficEstimation();
	assignHosts2IDS();
	msg(MSG_INFO, "updatePriorities");
	updatePriorities();
	msg(MSG_INFO, "setIpConfig");
	setIpConfig();
	gettimeofday(&roundStart, 0);
	round++;
}

void PriorityPacketSelector::start()
{
	gettimeofday(&startTime, 0);

	// send template to succeeding modules
	if (flowExporter) {
		IpfixTemplateRecord* record = templateRecordIM.getNewInstance();
		record->templateInfo = templateInfo;
		flowExporter->receive(record);
	}
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

void PriorityPacketSelector::queueUtilization(uint32_t queueid, uint32_t maxsize, uint32_t cursize)
{
	volatile PacketHostInfo* phi = packetHostInfo[queueid];
	if (cursize>phi->critQueueSize && phi->maxHostId > 1) {
		// queue too full, increase modulo ratio

		msg(MSG_INFO, "queue id %u, critQueueSize: %u, cursize: %u, max host id: %u, host count: %u",
				queueid, phi->critQueueSize, phi->maxHostId, phi->hostCount);
		if (phi->maxHostId == 1) {
			if (!phi->minWarningShown) {
				phi->minWarningShown = true;
				msg(MSG_INFO, "maxHostId for queue id %u already at minimum (==1)!", queueid);
			}
			return;
		}
		phi->critQueueSize = maxsize-((maxsize-phi->critQueueSize)>>1);
		packetHostInfo[queueid]->maxHostId >>= 1;
		msg(MSG_INFO, "increasing drop ratio for queue id %u to %.3f", queueid, (double)phi->hostCount/phi->maxHostId);
	} else {
		// reset critical queue size again
		if (phi->critQueueSize>(maxsize>>1)) {
			uint32_t cqs = maxsize-((maxsize-phi->critQueueSize)<<1);
			if (cursize<cqs)
				phi->critQueueSize = cqs;
		}
	}
}

bool PriorityPacketSelector::wasHostDropped(HostData* host)
{
	if (!packetHostInfo || host->assignedIdsId<0 || !packetHostInfo[host->assignedIdsId])
		return false;
	return (host->id > packetHostInfo[host->assignedIdsId]->maxHostId);
}

void PriorityPacketSelector::setFlowExporter(Destination<IpfixRecord*>* di)
{
	flowExporter = di;
}

void PriorityPacketSelector::setQueueSizes(vector<uint32_t> queuesizes)
{
	queueSizes = queuesizes;
}
