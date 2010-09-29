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

#ifndef _STATE_CONNECITON_FILTER_H_
#define _STATE_CONNECTION_FILTER_H_

#include <modules/packet/filter//PacketProcessor.h>
#include <common/bloom/BloomFilter.h>

#include <map>
#include <ostream>

class MemStatistics;

class StateConnectionFilter : public PacketProcessor {
public:
	StateConnectionFilter(unsigned timeout, unsigned bytes);
	~StateConnectionFilter();

	bool processPacket(Packet* p, bool connFilterResult);
	virtual bool processPacket(Packet* p);
	
	void setExportControlPackets(bool e) { exportControlPackets = e; }
protected:
	unsigned timeout;
	unsigned exportBytes;
	std::map<QuintupleKey, int> exportList;
	bool exportControlPackets;
};

#endif
