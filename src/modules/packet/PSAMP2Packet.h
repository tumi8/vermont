/*
 * VERMONT
 * Copyright (C) 2009 Daniel Wilhelm <e-mail@jiaz.de>
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

#if !defined(PSAMP2PACKET_H)
#define PSAMP2PACKET_H

#include "modules/ipfix/IpfixRecordDestination.h"
#include "modules/ipfix/Connection.h"
#include "common/ConcurrentQueue.h"
#include "core/Source.h"
#include "core/Module.h"
#include "Packet.h"

#include <list>
#include <string>

using namespace std;

class PSAMP2Packet: public Module,
		public IpfixRecordDestination,
		public Source<Packet*> {
public:
	PSAMP2Packet();
	virtual ~PSAMP2Packet();
	virtual void onDataRecord(IpfixDataRecord* record);
	virtual string getStatisticsXML(double interval);

private:
	struct timeval timestamp;

	/*counters for statistics*/
	uint64_t recievedDataRecords;
	uint64_t validPSAMP;

	/*temporary packetmemory to assemble the packet for packet->init(),65kb is max size*/
	uint8_t packet[66000];

	/*our own InstanceManager for packets because we are connected to an ipfixobserver*/
	static InstanceManager<Packet> packetManager;
};

#endif
