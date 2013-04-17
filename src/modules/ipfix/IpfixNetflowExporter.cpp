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

#include "IpfixNetflowExporter.h"
#include "core/Timer.h"
#include "common/Time.h"
#include "Connection.h"

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>



IpfixNetflowExporter::IpfixNetflowExporter(string hostname, uint16_t port, uint32_t maxrecordrate)
	: destPort(port),
	  timeoutRegistered(false),
	  recordsAlreadySent(false),
	  maxRecordRate(maxrecordrate),
	  recordCacheTimeout(INE_DEFAULT_RECORDCACHETIMEOUT)
{
	nextTimeout.tv_sec = 0;
	nextTimeout.tv_nsec = 0;
	curTimeStep.tv_sec = 0;
	curTimeStep.tv_usec = 0;

	packet.header.init();

	siDest.sin_family = AF_INET;
	siDest.sin_port = htons(destPort);
	struct hostent* host = gethostbyname(hostname.c_str());
	if (host==NULL) {
		THROWEXCEPTION("IpfixNetflowExporter: failed to resolve host '%s' (%s)", hostname.c_str(), strerror(errno));
	}
	siDest.sin_addr = *reinterpret_cast<struct in_addr*>(host->h_addr_list[0]);
	gettimeofday(&sysUptime, 0);
}

IpfixNetflowExporter::~IpfixNetflowExporter()
{
}


/**
 * gets called regularly to send data over the network
 */
void IpfixNetflowExporter::onTimeout(void* dataPtr)
{
	timeoutRegistered = false;

	if (recordsAlreadySent) {
		timeval tv;
		gettimeofday(&tv, 0);
		if (nextTimeout.tv_sec>tv.tv_sec || (nextTimeout.tv_sec==tv.tv_sec && nextTimeout.tv_nsec>tv.tv_usec*1000)) {
			// next timeout is in the future, reregister it
			timer->addTimeout(this, nextTimeout, NULL);
			// as the next timeout is not over yet, we don't need to send the records
			return;
		}
	}
	sendRecords(true);
}

/**
 * registers timeout for function onTimeout in Timer
 * (used to send records which are cached)
 */
void IpfixNetflowExporter::registerTimeout()
{
	if (timeoutRegistered) return;

	addToCurTime(&nextTimeout, recordCacheTimeout);
	timer->addTimeout(this, nextTimeout, NULL);
	timeoutRegistered = true;
}

void IpfixNetflowExporter::performStart()
{
	if ((sockfd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1) {
		THROWEXCEPTION("IpfixNetflowExporter: failed to set up UDP socket (%s)", strerror(errno));
	}

}

/**
 * sends all cached records
 */
void IpfixNetflowExporter::performShutdown()
{
	sendRecords(true);
	close(sockfd);
}

/**
 * during reconfiguration ensure, that all cached flows are exported
 */
void IpfixNetflowExporter::onReconfiguration1()
{
	sendRecords(true);
}

/**
 * Terminates and sends a network packet if available.
 * @return returns -1 on error, 0 otherwise
 */
void IpfixNetflowExporter::sendPacket()
{
	if(recordCache.size() > 0) {
		// determine if we need to wait (we don't want to exceed the defined packet rate per second)
		// check in 100ms steps if maximum packet rate is reached - if yes, wait until the 100ms step
		// is over
		// only do this whenever maxRecordRate is > 0
		struct timeval tv;
		gettimeofday(&tv, 0);
		if (maxRecordRate > 0) {
			struct timeval tv;
			gettimeofday(&tv, 0);
			if ((tv.tv_sec==curTimeStep.tv_sec) && (tv.tv_usec/100000==curTimeStep.tv_usec/100000)) {
				if (recordsSentStep>maxRecordRate/10) {
					// wait until current timestep is over
					usleep(100000-(tv.tv_usec%100000));
				}
			} else {
				curTimeStep = tv;
				recordsSentStep = 0;
			}
		}

		for (uint32_t i=0; i<NF5_MAXRECORDS; i++) packet.record[i].init();
		uint32_t count = 0;
		while (count<NF5_MAXRECORDS && !recordCache.empty()) {
			IpfixDataRecord* record = recordCache.front();
			recordCache.pop();
			Connection c(record);
			NetflowV5DataRecord* r = &packet.record[count];
			r->srcaddr = c.srcIP;
			r->dstaddr = c.dstIP;
			r->dPkts = htonl(ntohll(c.srcPackets) & 0xFFFFFFFF);
			r->dOctets = htonl(ntohll(c.srcOctets) & 0xFFFFFFFF);
			struct timeval flowtime;
			struct timeval timediff;
			flowtime.tv_sec = c.srcTimeStart/1000 & 0xFFFFFFFF;
			flowtime.tv_usec = (c.srcTimeStart%1000)*1000;
			timeval_subtract(&timediff, &flowtime, &sysUptime);
			r->first = htonl(timediff.tv_sec*1000+timediff.tv_usec/1000);
			flowtime.tv_sec = c.srcTimeEnd/1000 & 0xFFFFFFFF;
			flowtime.tv_usec = (c.srcTimeEnd%1000)*1000;
			timeval_subtract(&timediff, &flowtime, &sysUptime);
			r->last = htonl(timediff.tv_sec*1000+timediff.tv_usec/1000);
			r->srcport = c.srcPort;
			r->dstport = c.dstPort;
			r->tcp_flags = c.srcTcpControlBits;
			r->prot = c.protocol;
			record->removeReference();
			count++;
		}
		msg(MSG_DEBUG, "sending Netflow.v5 packet, flow count: %hu", count);
		packet.header.count = htons(count);
		packet.header.unixSec = htonl(tv.tv_sec);
		packet.header.unixNanoSec = htonl(tv.tv_usec*1000);
		packet.header.flowSeqNr = htonl(flowSeqNr);
		flowSeqNr += count;
		struct timeval difftime;
		timeval_subtract(&difftime, &tv, &sysUptime);
		packet.header.sysUptime = htonl(difftime.tv_sec*1000+difftime.tv_usec/1000);

		uint16_t packetsize = sizeof(NetflowV5Header)+count*sizeof(NetflowV5DataRecord);
		if (sendto(sockfd, &packet, packetsize, 0, (struct sockaddr*)(&siDest), sizeof(siDest))==-1) {
			msg(MSG_ERROR, "IpfixNetflowExporter: WARNING, failed to send UDP packet (%s)", strerror(errno));
		}
	}
}

/**
 * sends records to the network
 * @param forcesend to send all records regardless how many were cached
 */
void IpfixNetflowExporter::sendRecords(bool forcesend)
{
	if (recordCache.size() == 0) return;

	// we have a maximum of 30 data records in one packet
	if ((recordCache.size() >= 30) || forcesend) {
		sendPacket();
	}
	// set next timeout
	addToCurTime(&nextTimeout, recordCacheTimeout);
}

/**
 * Put new Data Record in outbound exporter queue
 * @param rec Data Data Record
 */
void IpfixNetflowExporter::onDataRecord(IpfixDataRecord* record)
{
	registerTimeout();
	recordCache.push(record);
	sendRecords();
}
