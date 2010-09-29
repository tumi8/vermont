/*
 * Vermont Packet Filter
 * Copyright (C) 2009  Vermont Project
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

/*
 * PSAMP Reference Implementation
 *
 * PacketProcessor.h
 *
 * Base class for a packet processor (filter/sampler)
 *
 * Author: Michael Drueing <michael@drueing.de>
 *
 */

#ifndef PACKET_PROCESSOR_H
#define PACKET_PROCESSOR_H

#include "modules/packet/Packet.h"

#include <string>

class PacketProcessor
{
public:
	/*
	 processPacket(Packet *)
	 processes a packet and returns true if the packet passes the
	 filter/sampler and false if it should be dropped
	*/
	virtual bool processPacket(Packet *p) = 0;

	virtual ~PacketProcessor() { };

	virtual std::string getStatisticsXML(double interval) { return ""; }
};

#endif
