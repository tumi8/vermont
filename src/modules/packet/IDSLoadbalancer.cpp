/*
 * Vermont IDS Load Balancer
 * Copyright (C) 2010 Vermont Project
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
 *
 */

#include "IDSLoadbalancer.hpp"

#include "modules/packet/Packet.h"
#include "modules/packet/PCAPExporterPipe.h"
#include "common/Time.h"
#include "idsloadbalancer/PriorityPacketSelector.hpp"

#include <boost/lexical_cast.hpp>
#include <iostream>
#include <list>

using namespace std;

IDSLoadbalancer::IDSLoadbalancer(std::string &_selector, uint64_t updateinterval)
	: selector(NULL),
	  selectorAsString(_selector),
	  qcount(0),
	  thread(threadWrapper),
	  shutdownThread(false),
	  updateInterval(updateinterval)
{
	if (_selector == "IpPacketSelector"){
		selector = new IpPacketSelector();
	} else if (_selector == "HashPacketSelector"){
		selector = new HashPacketSelector();
	} else if (_selector == "PriorityPacketSelector"){
		//selector = new PriorityPacketSelector(); FIXME
	} else {
		THROWEXCEPTION("Invalid selector");
	}
}

IDSLoadbalancer::~IDSLoadbalancer()
{
}
/**
 * Startup method for the module
 */
void IDSLoadbalancer::performStart()
{
	//SignalHandler::getInstance().registerSignalHandler(SIGPIPE, this);

	msg(MSG_INFO, "Started IDSLoadbalancer with the following parameters:");
	msg(MSG_INFO, "  - PacketSelector = %s", selectorAsString.c_str());
	qcount = getSucceedingModuleCount();
	selector->setQueueCount(qcount);
	msg(MSG_INFO, "  - QueueCount = %d", qcount);

	shutdownThread = false;
	thread.run(this);
}

void IDSLoadbalancer::performShutdown()
{
	shutdownThread = true;
	thread.join();
}


void IDSLoadbalancer::receive(Packet* packet)
{
	int res = selector->decide(packet);
	if (res == -1){
		DPRINTFL(MSG_VDEBUG, "Dropping packet");
		return;
	}

	send(packet, res);
}

void IDSLoadbalancer::forwardPacket(Packet* packet, int queue)
{
	//msg(MSG_INFO, "Forwarding packet to queue %d", queue);
}

void IDSLoadbalancer::setIpConfig(std::map<uint32_t, int> & s, std::map<uint32_t, int> &d)
{
	static_cast<IpPacketSelector*>(selector)->initializeConfig(s, d);
	//src = s;
	//dst = d;
}

void* IDSLoadbalancer::threadWrapper(void* data)
{
	IDSLoadbalancer* ilb = reinterpret_cast<IDSLoadbalancer*>(data);
	ilb->registerCurrentThread();
	ilb->updateWorker();
	ilb->unregisterCurrentThread();
	return NULL;
}

void IDSLoadbalancer::updateWorker()
{
	struct timespec ts;
	addToCurTime(&ts, updateInterval);
	while (!shutdownThread) {
		updateBalancingLists();

		while (nanosleep(&ts, 0) != 0 && !shutdownThread) {}
		addToCurTime(&ts, updateInterval);
	}
}

void IDSLoadbalancer::updateBalancingLists()
{
	list<IDSLoadStatistics> stats(qcount);

	// get load data from succeeding modules
	for (uint32_t i=0; i<qcount; i++) {
		Destination<Packet*>* dp = getSucceedingModuleInstance(i);
		ProcessStatisticsProvider* psp = dynamic_cast<ProcessStatisticsProvider*>(dp);
		if (!psp) THROWEXCEPTION("IDSLoadBalancer: succeeding module #%u is not of type ProcessStatisticsProvider! Use module type PcapExporterPipe!", i);
		uint32_t ujiffies, sjiffies;
		uint64_t dpkts;
		psp->getDroppedPackets(dpkts);
		if (psp->getProcessStatistics(sjiffies, ujiffies)) {
			stats.push_back(IDSLoadStatistics(true, dpkts, sjiffies, ujiffies));
		} else {
			stats.push_back(IDSLoadStatistics(true, dpkts));
		}
	}
	selector->updateData(stats);
}
