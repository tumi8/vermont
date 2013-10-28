/*
 * Vermont Connection Filter
 * Copyright (C) 2008 Lothar Braun
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

#include "StateConnectionFilter.h"

#include <iostream>

StateConnectionFilter::StateConnectionFilter(unsigned timeout, unsigned bytes)
	: exportControlPackets(true)
{
	this->timeout = timeout;
	this->exportBytes = bytes;
	msg(MSG_INFO, "Created stateConnectionFilter with parameters:");
	msg(MSG_INFO, "\t - %i seconds timeout", timeout);
	msg(MSG_INFO, "\t - %i bytes to export", bytes);
}

StateConnectionFilter::~StateConnectionFilter()
{
}

bool StateConnectionFilter::processPacket(Packet* p)
{
	return processPacket(p, true);
}

bool StateConnectionFilter::processPacket(Packet* p, bool connFilterResult)
{
	unsigned flagsOffset = p->transportHeaderOffset + 13;
	static const uint8_t SYN = 0x02;
	static const uint8_t FIN = 0x01;
	static const uint8_t RST = 0x04;
	unsigned payloadLen;
	if (p->classification & PCLASS_PAYLOAD)
		payloadLen = p->data_length - p->payloadOffset;
	else
		payloadLen = 0;

	if (p->ipProtocolType != Packet::TCP) {
		DPRINTF("Got a non-TCP packet. Protocol-type is %i", p->ipProtocolType);
		return false;
	}

	QuintupleKey key(p);

	if (*((uint8_t*)p->data.netHeader + flagsOffset) & SYN) {
		DPRINTF("StateConnectionFilter: Got SYN packet");
		if (exportList.find(key) == exportList.end()) {
			exportList[key] = 0;
		}
		return exportControlPackets;
	} else if (*((uint8_t*)p->data.netHeader + flagsOffset) & RST || *((uint8_t*)p->data.netHeader + flagsOffset) & FIN) {
		DPRINTF("StateConnectionFilter: Got %s packet", *((uint8_t*)p->data.netHeader + flagsOffset) & RST?"RST":"FIN");
		if (exportList.find(key) != exportList.end()) {
			exportList.erase(exportList.find(key));
		}
		return exportControlPackets;
	} else {
		DPRINTF("StateConnectionFilter: Got a normal packet");
		if (exportList.find(key) == exportList.end()) {
			// unknown connection
			return false;
		} else if (exportList[key] > 0) {
			bool ret = exportList[key]>static_cast<int>(payloadLen)?true:false;
			DPRINTF("StateConnectionFilter: Connection known, exporting packet");
			exportList[key] -= payloadLen;
			DPRINTF("StateConnectionFilter: We have to export %i bytes after exporting this packet", exportList[key]>0?exportList[key]:0);
			if (exportList[key] <= 0) {
				exportList.erase(exportList.find(key));
			}
			return ret;
		} else {
			// new established connection
			bool ret = exportBytes>payloadLen?true:false;
			if (exportBytes > payloadLen) {
				exportList[key] = exportBytes - payloadLen;
			} else {
				exportList.erase(exportList.find(key));
			}
			return ret;
		}
	}

	msg(MSG_FATAL, "StateConnectionFilter: SOMTHING IS SCRWED UP, YOU SHOULD NEVER SEE THIS MESSAGE!");
	return false; // make compiler happy
}


