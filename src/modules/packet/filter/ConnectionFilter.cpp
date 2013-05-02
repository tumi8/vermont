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

#ifdef HAVE_CONNECTION_FILTER

#include "ConnectionFilter.h"

ConnectionFilter::ConnectionFilter(unsigned Timeout, unsigned bytes, unsigned hashFunctions, unsigned filterSize)
	:  hashParams(hashFunctions), synFilter(&hashParams, filterSize, false), exportFilter(&hashParams, filterSize, false),
	  connectionFilter(&hashParams, filterSize, false), timeout(Timeout), exportBytes(bytes), exportControlPackets(true)
{
	msg(MSG_INFO, "Created connectionFilter with parameters:");
	msg(MSG_INFO, "\t - %i seconds timeout", timeout);
	msg(MSG_INFO, "\t - %i bytes filter size", filterSize);
	msg(MSG_INFO, "\t - %i hash functions", hashFunctions);
	msg(MSG_INFO, "\t - %i bytes to export", bytes);
}

ConnectionFilter::ConnectionFilter(unsigned Timeout, unsigned bytes, unsigned hashFunctions, unsigned filterSize, unsigned seed)
	: hashParams(hashFunctions, seed), synFilter(&hashParams, filterSize, false), exportFilter(&hashParams, filterSize, false),
	connectionFilter(&hashParams, filterSize, false), timeout(Timeout), exportBytes(bytes), exportControlPackets(true)
{
	msg(MSG_INFO, "Created connectionFilter with parameters:");
	msg(MSG_INFO, "\t - %i seed", seed);
	msg(MSG_INFO, "\t - %i seconds timeout", timeout);
	msg(MSG_INFO, "\t - %i bytes filter size", filterSize);
	msg(MSG_INFO, "\t - %i hash functions", hashFunctions);
	msg(MSG_INFO, "\t - %i bytes to export", bytes);
}

bool ConnectionFilter::processPacket(Packet* p)
{
	unsigned flagsOffset = p->transportHeaderOffset + 13;
	static const uint8_t SYN = 0x02;
	static const uint8_t FIN = 0x01;
	static const uint8_t RST = 0x04;
	static unsigned long tmp;
	QuintupleKey key(p);
	unsigned payloadLen;
	if (p->classification & PCLASS_PAYLOAD)
		payloadLen = p->data_length - p->payloadOffset;
	else
		payloadLen = 0;

	if (p->ipProtocolType != Packet::TCP) {
		DPRINTF("Got a non-TCP packet. Protocol-type is %i", p->ipProtocolType);
		return false;
	}

	if (*((uint8_t*)p->layer2Start + flagsOffset) & SYN) {
		DPRINTF("ConnectionFilter: Got SYN packet");
		synFilter.set(key.data, key.len, (agetime_t)p->timestamp.tv_sec);
		DPRINTF("ConnectionFilter: synFilter saved time %u", synFilter.get(key.data, key.len));
		return exportControlPackets;
	} else if (*((uint8_t*)p->layer2Start + flagsOffset) & RST || *((uint8_t*)p->layer2Start + flagsOffset) & FIN) {
		
		DPRINTF("ConnectionFilter: Got %s packet", *((uint8_t*)p->layer2Start + flagsOffset) & RST?"RST":"FIN");
	
		exportFilter.set(key.data, key.len, -exportFilter.get(key.data, key.len));
		connectionFilter.set(key.data, key.len, p->timestamp.tv_sec);
		DPRINTF("ConnectionFilter: connectionFilter saved time %u", connectionFilter.get(key.data, key.len));

		return exportControlPackets;
	} else {
		DPRINTF("ConnectionFilter: Got a normal packet");
		if ((tmp = exportFilter.get(key.data, key.len)) > 0) {
			DPRINTF("ConnectionFilter: Connection known, exporting packet");
			static unsigned diffVal;
			bool ret = false;
			if (tmp > payloadLen) {
				diffVal = -payloadLen;
				ret = true;
			}
			else {
				diffVal = -tmp;
				ret = false;
			}
			exportFilter.set(key.data, key.len, diffVal);
			if (exportFilter.get(key.data, key.len) <= 0) {
				connectionFilter.set(key.data, key.len, p->timestamp.tv_sec);
			}
			DPRINTF("ConnectionFilter: We have to export %i bytes after exporting this packet", exportFilter.get(key.data, key.len));
			return ret;
		} else {
			if ((unsigned)(p->timestamp.tv_sec - synFilter.get(key.data, key.len)) < timeout &&
			    synFilter.get(key.data, key.len) - connectionFilter.get(key.data, key.len) > 0) {
				bool ret = false;
			    	DPRINTF("ConnectionFilter: Found new connection, exporting packet");
				if (payloadLen < exportBytes) {
					exportFilter.set(key.data, key.len, exportBytes - payloadLen);
					ret = true;
				} else {
					connectionFilter.set(key.data, key.len, p->timestamp.tv_sec);
					ret = false;
				}
				DPRINTF("ConnectionFilter: We have to export %i bytes after exporting this packet", exportFilter.get(key.data, key.len));
				return ret;
			}
			DPRINTF("ConnectionFilter: Paket will not be exported");
			return false;
		}
	}

	msg(MSG_FATAL, "ConnectionFilter: SOMTHING IS SCRWED UP, YOU SHOULD NEVER SEE THIS MESSAGE!");
	return false; // make compiler happy
}

#endif
