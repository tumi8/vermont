/*
 * Vermont PCAP Exporter
 * Copyright (C) 2009 Vermont Project
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

#ifndef IDS_LOADBALANCER_HPP
#define IDS_LOADBALANCER_HPP

#include "core/Module.h"
#include "HashPacketSelector.hpp"
#include "IpPacketSelector.hpp"

#include <common/msg.h>

#include <string>
#include <ctime>
#include <common/SignalInterface.h>

class Packet;


class IDSLoadbalancer : public Module, public Destination<Packet *>, public Source<Packet *> 
{
public:
	IDSLoadbalancer(std::string &_selector, int count);
	~IDSLoadbalancer();
	virtual void receive(Packet* packet);
	void setIpConfig(std::map<uint32_t, int> &, std::map<uint32_t, int> &);
	void performStart();
	void performShutdown();
	void forwardPacket(Packet* packet, int queue);

private:
	BasePacketSelector *selector;
	std::string selectorAsString;
	int qcount;
	std::map<uint32_t, int> src;
	std::map<uint32_t, int> dst;
};

#endif
