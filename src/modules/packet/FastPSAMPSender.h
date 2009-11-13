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

#ifndef FASTPSAMPSENDER_H_
#define FASTPSAMPSENDER_H_
#include "modules/packet/Packet.h"

/**
 * defines the index of exporttime in IpFixHeader
 */
#define FPSAMP_EXPORTTIME_INDEX 4
/**
 * defines the index of flowsequence in IpFixHeader
 */
#define FPSAMP_FLOWSEQUENCE_INDEX 8
/**
 * defines default size of IpFix Template for FastPSAMPExporer
 */
#define FPSAMP_TEMPLATE_SIZE 28

/**
 * defines the index of length in Header
 */
#define FPSAMP_DATALENGTH_INDEX 2

/**
 * defines max payload size of IpFix Packet
 */
#define FPSAMP_MAX_PAYLOAD 65500

/**
 * defines headerlength of IpFixHeader
 */
#define FPSAMP_HEADER_LENGTH 16

/**
 * defines RFC 5101, Variable Length Information Element, optimized lenght information
 */
#define FPSAMP_VLENGTH 255
#define FPSAMP_VLENTH_SMALLER255_SIZE 5
#define FPSAMP_VLENTH_SMALLER255_INDEX 4
#define FPSAMP_VLENTH_LARGER255_SIZE 7
#define FPSAMP_VLENTH_LARGER255_INDEX 5

class FastPSAMPSender {
public:
	FastPSAMPSender(const std::string& ip, uint16_t port, bool udpORsctp);
	virtual ~FastPSAMPSender();
	virtual std::string getStatisticsXML(double interval);
	/*change Timeout for Template with packetcounter*/
	void setsendTemplateeveryPacket(uint64_t x);
	/*change Timeout for Template with elapsed time*/
	void setsendTemplateeverySecs(uint64_t x);
	/*processes a recieved packet*/
	void handlePacket(Packet *p);
private:
	/*send IpFixTemplate*/
	void sendTemplate();
	/*send DataRecord with PSAMP content*/
	void sendDataRecord(Packet *p);
	/*init our UDP/SCTP socket for sending data*/
	void initSocket();
	/*handle important connection issues*/
	void handleConnectionIssues();

	/*optional timeout for template sending*/
	uint64_t sendTemplateafterPacket;
	uint64_t sendTemplateeveryPacket;
	uint64_t sendTemplateafterSecs;
	uint64_t sendTemplateeverySecs;

	/*counters*/
	uint64_t recievedPackets;
	uint64_t sendDataRecords;
	uint64_t sendTemplates;
	uint64_t oversizedPackets;
	uint64_t failedWrites;

	/*flowsequence and its networkorder*/
	uint32_t flowsequence;
	uint32_t flowsequence_networkorder;

	/*current timestamp and its networkorder*/
	struct timeval timestamp;
	uint32_t exporttime;

	/*buffers to hold Template and PSAMP Information Element*/
	uint8_t *sendbuffer_1;
	uint8_t sendbuffer_2[7];

	/*iovector for writev*/
	struct iovec iov[3];

	/*some temp variables*/
	uint16_t payloadsize;
	uint16_t tempsize;

	/*network stuff*/
	bool connected;
	int sock;
	sockaddr serveraddrcast;
	struct sockaddr_in addr;
	int addrsize;
	bool useSCTP;
	string dstip;
	uint16_t dstport;
};

#endif /* FASTPSAMPSENDER_H_ */
