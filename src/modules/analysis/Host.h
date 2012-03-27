/*
 * VERMONT
 * Copyright (C) 2012 Lothar Braun <braun@net.in.tum.de>
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

#ifndef _HOST_H_
#define _HOST_H_

#include "modules/ipfix/Connection.h"
#include "common/ManagedInstance.h"

class Host : public ManagedInstance<Host> {
public:
	Host(InstanceManager<Host>* im);
	~Host();
	void setIP(uint32_t address) { ip = address; }

	void addConnection(Connection* c);

	uint32_t ip;

	uint32_t answeredFlows;
	uint32_t unansweredFlows;
	
	uint64_t sentBytes;
	uint64_t sentPackets;
	uint64_t recBytes;
	uint64_t recPackets;

	uint64_t recHighPorts;
	uint64_t sentHighPorts;
	uint64_t recLowPorts;
	uint64_t sentLowPorts;

	uint64_t lastSeen;
};

#endif
