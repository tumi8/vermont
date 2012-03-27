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

#include "Host.h"


Host::Host(InstanceManager<Host>* im)
	: ManagedInstance<Host>(im), ip(0), answeredFlows(0), unansweredFlows(0), sentBytes(0),
	  sentPackets(0), recBytes(0), recPackets(0), recHighPorts(0),
	  sentHighPorts(0), recLowPorts(0), sentLowPorts(0), lastSeen(0)
{

}

Host::~Host()
{

}

void Host::addConnection(Connection* c)
{
	if (c->srcIP != ip && c->dstIP != ip) {
		msg(MSG_ERROR, "Host: Received a connection that from %u to %u. However, my IP address is %u", c->srcIP, c->dstIP, ip);
		return;
	}

	// update counter of successful vs. unsuccessful connections
	if (c->srcOctets != 0 && c->dstOctets != 0) {
		answeredFlows++;
	} else {
		unansweredFlows++;
	}
	
	if (c->srcIP == ip) {
		// host is a client
		sentPackets += ntohll(c->srcPackets);
		sentBytes   += ntohll(c->srcOctets);
		recPackets  += ntohll(c->dstPackets);
		recBytes    += ntohll(c->dstOctets);
		// check for  privileged port
		if (ntohs(c->srcPort) < 1024) {
			sentLowPorts++;
		} else {
			sentHighPorts++;
		}
		if (ntohs(c->dstPort) < 1024) {
			recLowPorts++;
		} else {
			recHighPorts++;
		}
		lastSeen = c->srcTimeEnd;
	} else {
		sentPackets += ntohll(c->dstPackets);
		sentBytes   += ntohll(c->dstOctets);
		recPackets  += ntohll(c->srcPackets);
		recBytes    += ntohll(c->srcOctets);
		// chck for privileged ports
		if (ntohs(c->srcPort) < 1024) {
			recLowPorts++;
		} else {
			recHighPorts++;
		}
		if (ntohs(c->dstPort) < 1024) {
			sentLowPorts++;
		} else {
			sentHighPorts++;
		}
		lastSeen = c->dstTimeEnd;
	}
}
