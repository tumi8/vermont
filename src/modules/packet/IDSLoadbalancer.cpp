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
#include <boost/lexical_cast.hpp>
#include <iostream>

IDSLoadbalancer::IDSLoadbalancer(std::string &_selector, int count) 
	: selector(NULL), selectorAsString(_selector), qcount(count)
{
	if (_selector == "IpPacketSelector"){
		selector = new IpPacketSelector();
	} else if (_selector == "HashPacketSelector"){
		selector = new HashPacketSelector();
	} else {
		THROWEXCEPTION("Invalid selector");
	}
	selector->setNumberOfQueues(qcount);
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
	msg(MSG_INFO, "  - QueueCount = %d", qcount);
}

void IDSLoadbalancer::performShutdown()
{
	//SignalHandler::getInstance().unregisterSignalHandler(SIGCHLD, this);
}


void IDSLoadbalancer::receive(Packet* packet)
{
	int res = selector->decide(packet);
	if (res == -1){
		DPRINTFL(MSG_VDEBUG, "Dropping packet");
		return;
	}	
// 	if(qcount == 1)
// 		send(packet);
// 	else
		send(packet,res);
	//forwardPacket(packet, res);
}

void IDSLoadbalancer::forwardPacket(Packet* packet, int queue){
	//msg(MSG_INFO, "Forwarding packet to queue %d", queue);
}

void IDSLoadbalancer::setIpConfig(std::map<uint32_t, int> & s, std::map<uint32_t, int> &d){
	static_cast<IpPacketSelector*>(selector)->initializeConfig(s, d);
	//src = s;
	//dst = d;
}
