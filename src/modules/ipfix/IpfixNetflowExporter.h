/*
 * IPFIX Netflow Exporter
 * Copyright (C) 2010 Tobias Limmer
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

#ifndef IPFIXNETFLOWEXPORTER_H
#define IPFIXNETFLOWEXPORTER_H

#include "core/Module.h"
#include "core/Notifiable.h"
#include "IpfixRecordDestination.h"

#include <strings.h>


#define NF5_MAXRECORDS 30

/**
 * IPFIX Netflow exporter module
 *
 * Exports all received flows in Netflow.v5 format, filling
 * non-existent fields with 0
 *
 */
class IpfixNetflowExporter : public Module, public Source<NullEmitable*>, public IpfixRecordDestination, public Notifiable
{
public:
	IpfixNetflowExporter(string hostname, uint16_t port, uint32_t maxrecordrate);
	virtual ~IpfixNetflowExporter();

	void flushPacket();

	// inherited from IpfixRecordDestination
	virtual void onDataRecord(IpfixDataRecord* record);

	virtual void onReconfiguration1();

	// inherited from Notifiable
	virtual void onTimeout(void* dataPtr);

protected:

	void performStart();
	void performShutdown();
	void sendPacket();
	void sendRecords(bool forcesend = false);
	void registerTimeout();


private:

#pragma pack(push)
#pragma pack(1)
	struct NetflowV5Header
	{
		uint16_t version;
		uint16_t count;
		uint32_t sysUptime;
		uint32_t unixSec;
		uint32_t unixNanoSec;
		uint32_t flowSeqNr;
		uint8_t engineType;
		uint8_t engineID;
		uint16_t samplInfo;

		void init() {
			bzero(this, sizeof(NetflowV5Packet));
			version = htons(5);
		}
	};

	struct NetflowV5DataRecord
	{
		uint32_t srcaddr;
		uint32_t dstaddr;
		uint32_t nexthop;
		uint16_t input;
		uint16_t output;
		uint32_t dPkts;
		uint32_t dOctets;
		uint32_t first;
		uint32_t last;
		uint16_t srcport;
		uint16_t dstport;
		uint8_t pad1;
		uint8_t tcp_flags;
		uint8_t prot;
		uint8_t tos;
		uint16_t src_as;
		uint16_t dst_as;
		uint8_t src_mask;
		uint8_t dst_mask;
		uint16_t pad2;

		void init() {
			bzero(this, sizeof(NetflowV5DataRecord));
		}
	};

	struct NetflowV5Packet
	{
		NetflowV5Header header;
		NetflowV5DataRecord record[NF5_MAXRECORDS];
	};
#pragma pack(pop)


	int sockfd;
	struct sockaddr_in siDest;
	uint16_t destPort;
	queue<IpfixDataRecord*> recordCache;
	timespec nextTimeout;
	bool timeoutRegistered; /**< true if next timeout was already registered in timer */
	bool recordsAlreadySent; /**< true if records were sent to the network as the packet was full */
	struct timeval sysUptime; /**< time when system was started */
	struct timeval curTimeStep; /**< current time used for determining packet rate */
	uint32_t recordsSentStep; /**< number of records sent in timestep (usually 100ms)*/
	uint32_t maxRecordRate;  /** maximum number of records per seconds to be sent over the wire */
	uint16_t recordCacheTimeout; /**< how long may records be cached until sent, milliseconds */
	uint32_t flowSeqNr; /**< flow sequence number for .v5 header */

	NetflowV5Packet packet; /**< pointer to buffer space where packet is assembled */
};

#endif
