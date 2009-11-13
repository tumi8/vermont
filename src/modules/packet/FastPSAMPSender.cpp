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

#include "FastPSAMPSender.h"
#include <sys/time.h>
#include <time.h>

#include "common/msg.h"
#include "common/Misc.h"
#include "common/ipfixlolib/ipfixlolib.h"

FastPSAMPSender::FastPSAMPSender(const std::string& ip, uint16_t port,
		bool udpORsctp) :
	sendTemplateeveryPacket(0), sendTemplateeverySecs(0), recievedPackets(0),
			sendDataRecords(0), sendTemplates(0), oversizedPackets(0),
			failedWrites(0), flowsequence(0), connected(false), sock(-1) {

	/*temporary templatepacket containing IpFix Template for FastPSAMPExporter*/
	const uint8_t templatepacket[] = { 0x00, 0x0a, 0x00, 0x1c, 0x4a, 0xf3,
			0x5e, 0x4f, 0x00, 0x00, 0x94, 0x82, 0x00, 0x00, 0x00, 0x7b, 0x00,
			0x02, 0x00, 0x0c, 0x03, 0x78, 0x00, 0x01, 0x01, 0x39, 0xff, 0xff };
	/*create and fill sendbuffer_1 with IpFixHeader and Template for FastPSAMPExporter*/
	sendbuffer_1 = new uint8_t[sizeof(templatepacket)];
	memcpy(sendbuffer_1, templatepacket, sizeof(templatepacket));

	/*fill sendbuffer_2 with DataRecordHeader*/
	sendbuffer_2[0] = 0x03;
	sendbuffer_2[1] = 0x78;

	/*init network stuff*/
	memset(&(addr), 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip.c_str());
	serveraddrcast = (sockaddr &) addr;
	dstip = ip;
	dstport = port;
	useSCTP = udpORsctp;

	/*preinit io_vector for writev*/
	iov[0].iov_base = sendbuffer_1;
	iov[0].iov_len = FPSAMP_HEADER_LENGTH;
	iov[1].iov_base = sendbuffer_2;

	initSocket();
}

void FastPSAMPSender::initSocket() {
	if (sock < 0) {
		/*socket not created yet, lets do this right now*/
		if (useSCTP) {
			bool hasSCTP = false;
#ifdef SUPPORT_SCTP
			hasSCTP=true;
			if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_SCTP)) < 0) {
				msg(MSG_DIALOG, "could not create sctp socket", strerror(errno));
				sock=-1;
			} else {
				msg(MSG_DIALOG, "could  create sctp socket");
				addr.sin_family = PF_INET;
			}
#endif /*SUPPORT_SCTP*/
		} else {
			if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
				msg(MSG_DIALOG, "could not create udp socket", strerror(errno));
				sock = -1;
			} else {
				msg(MSG_DIALOG, "could  create udp socket");
				addr.sin_family = AF_INET;
			}
		}
	}
	if (sock == -1) {
		connected = false;
		return;
	}
	/*try to connect to socket*/
	if (connect(sock, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
		connected = false;
		msg(MSG_DIALOG,"could not open socket!,try again later");
	} else {
		connected = true;
		msg(MSG_DIALOG,"could open socket!");
	}
}

FastPSAMPSender::~FastPSAMPSender() {
	/*cleanup sendbuffer_1*/
	if (sendbuffer_1 != NULL) {
		delete[] sendbuffer_1;
		sendbuffer_1 = NULL;
	}
	/*close socket*/
	if (connected || sock > 0) {
		close(sock);
		sock = -1;
		connected = false;
	}
}

void FastPSAMPSender::handlePacket(Packet *p) {
	recievedPackets++;
	flowsequence++;
	flowsequence_networkorder = htonl(flowsequence);/*convert flowsequence to network byte order*/
	gettimeofday(&timestamp, NULL);
	exporttime = htonl(timestamp.tv_sec);/*convert timestamp to network byte order*/
	memcpy(sendbuffer_1 + FPSAMP_FLOWSEQUENCE_INDEX,
			&flowsequence_networkorder, 4);
	memcpy(sendbuffer_1 + FPSAMP_EXPORTTIME_INDEX, &exporttime, 4);
	if ((sendTemplateeveryPacket > 0 && sendTemplateafterPacket
			<= recievedPackets) || (sendTemplateeverySecs > 0
			&& sendTemplateafterSecs <= timestamp.tv_sec)) {
		sendTemplate();
	}
	sendDataRecord(p);
}

void FastPSAMPSender::sendTemplate() {
	/*correcting size of IpFixHeader for Template*/
	tempsize = htons(FPSAMP_TEMPLATE_SIZE);
	memcpy(sendbuffer_1 + FPSAMP_DATALENGTH_INDEX, &tempsize, 2);
	/*send template*/
	if (write(sock, sendbuffer_1, FPSAMP_TEMPLATE_SIZE) == -1) {
		handleConnectionIssues();
		failedWrites++;
		msg(MSG_ERROR, "could not send template %s", strerror(errno));
	} else {
		sendTemplates++;
	}
	/*set new timeout for template*/
	if (sendTemplateeveryPacket > 0)
		setsendTemplateeveryPacket(sendTemplateeveryPacket);
	if (sendTemplateeverySecs > 0)
		setsendTemplateeverySecs(sendTemplateeverySecs);
}

void FastPSAMPSender::sendDataRecord(Packet *p) {
	payloadsize = p->data_length - Packet::IPHeaderOffset;
	if (payloadsize > FPSAMP_MAX_PAYLOAD) {
		oversizedPackets++;
		msg(MSG_ERROR, "oversized packet");
		return;
	}
	/*RFC 5101, Variable Length Information Element*/
	if (payloadsize < FPSAMP_VLENGTH) {
		iov[1].iov_len = FPSAMP_VLENTH_SMALLER255_SIZE;
		/*correcting size of IpFixHeader*/
		tempsize = htons(FPSAMP_HEADER_LENGTH + FPSAMP_VLENTH_SMALLER255_SIZE
				+ payloadsize);
		memcpy(sendbuffer_1 + FPSAMP_DATALENGTH_INDEX, &tempsize, 2);
		/*correcting size of DataRecord*/
		tempsize = htons(FPSAMP_VLENTH_SMALLER255_SIZE + payloadsize);
		memcpy(sendbuffer_2 + FPSAMP_DATALENGTH_INDEX, &tempsize, 2);
		/*correcting size of DataRecord,Variable Length Information Element*/
		sendbuffer_2[FPSAMP_VLENTH_SMALLER255_INDEX] = payloadsize;
	} else {
		iov[1].iov_len = FPSAMP_VLENTH_LARGER255_SIZE;
		/*correcting size of IpFixHeader*/
		tempsize = htons(FPSAMP_HEADER_LENGTH + FPSAMP_VLENTH_LARGER255_SIZE
				+ payloadsize);
		memcpy(sendbuffer_1 + FPSAMP_DATALENGTH_INDEX, &tempsize, 2);
		/*correcting size of DataRecord*/
		tempsize = htons(FPSAMP_VLENTH_LARGER255_SIZE + payloadsize);
		memcpy(sendbuffer_2 + FPSAMP_DATALENGTH_INDEX, &tempsize, 2);
		/*correcting size of DataRecord,Variable Length Information Element*/
		sendbuffer_2[FPSAMP_VLENTH_SMALLER255_INDEX] = FPSAMP_VLENGTH;
		tempsize = htons(payloadsize);
		memcpy(sendbuffer_2 + FPSAMP_VLENTH_LARGER255_INDEX, &tempsize, 2);
	}
	/*set io_vector for current packet*/
	iov[2].iov_base = p->mmappeddata + Packet::IPHeaderOffset;
	iov[2].iov_len = payloadsize;
	/*send datarecord*/
	if (writev(sock, iov, 3) == -1) {
		handleConnectionIssues();
		failedWrites++;
		msg(MSG_ERROR, "could not send datarecord %s %i %i", strerror(errno),errno,sock);
	} else {
		/*increase sendDataRecords counter*/
		sendDataRecords++;
	}
}

void FastPSAMPSender::handleConnectionIssues() {
	if (errno == EISCONN || errno == ENOTCONN || errno == EPIPE || errno
			== EBADF) {
		/*connection broken, close it so we can try again*/
		if (connected || sock > 0) {
			close(sock);
			sock = -1;
			connected = false;
		}
	}
	if (!connected || sock < 0) {
		/*not connected, try to connect again*/
		initSocket();
	}
}

void FastPSAMPSender::setsendTemplateeveryPacket(uint64_t x) {
	sendTemplateeveryPacket = x;
	/*calculate next packetnumber to send next Template*/
	sendTemplateafterPacket = recievedPackets + sendTemplateeveryPacket;
}

void FastPSAMPSender::setsendTemplateeverySecs(uint64_t x) {
	sendTemplateeverySecs = x;
	/*calculate next time to send next Template*/
	sendTemplateafterSecs = timestamp.tv_sec + sendTemplateeverySecs;
}

string FastPSAMPSender::getStatisticsXML(double interval) {
	ostringstream oss;
	oss << "<fastpsampsender dest=\"" << dstip.c_str() << "\" port=\""
			<< dstport << "\">";
	oss << "<sendDataRecords>" << sendDataRecords << "</sendDataRecords>";
	oss << "<sendTemplates>" << sendTemplates << "</sendTemplates>";
	oss << "<recievedPackets>" << recievedPackets << "</recievedPackets>";
	oss << "<oversizedPackets>" << oversizedPackets << "</oversizedPackets>";
	oss << "<failedWrites>" << failedWrites << "</failedWrites>";
	oss << "</fastpsampsender>";
	return oss.str();
}
