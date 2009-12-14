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

#ifndef _CONNECTION_FILTER_H_
#define _CONNECTION_FILTER_H_

#include "PacketProcessor.h"

#include <common/bloom/CountBloomFilter.h>
#include <common/bloom/AgeBloomFilter.h>

class ConnectionFilter : public PacketProcessor {
public:
	ConnectionFilter(unsigned timeout, unsigned bytes, unsigned hashFunctions, unsigned FilterSize);
	ConnectionFilter(unsigned timeout, unsigned bytes, unsigned hashFunctions, unsigned FilterSize, unsigned seed);

	virtual bool processPacket(Packet* p);
	void setExportControlPackets(bool e) { exportControlPackets = e; }
protected:
	HashParams hashParams;
	AgeBloomFilter synFilter;
	CountBloomFilter exportFilter;
	AgeBloomFilter connectionFilter;
	unsigned timeout;
	unsigned exportBytes;
	bool exportControlPackets;
};

#endif

#endif
