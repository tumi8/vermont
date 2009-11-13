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

/**
 * This modul recieves an IpfixDataRecord and converts the PSAMP content (IP-Header+Payload) to a packet for future use
 */
#include "PSAMP2Packet.h"
#include "common/Misc.h"
#include "Template.h"

#include <math.h>

InstanceManager<Packet> PSAMP2Packet::packetManager("Packet");

PSAMP2Packet::PSAMP2Packet() :
	recievedDataRecords(0), validPSAMP(0) {
	/*fill LinkLayer(src/dst mac are zero, type is IP) in our temporary packet*/
	const uint8_t linklayer[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00 };
	memcpy(packet, linklayer, sizeof(linklayer));
}

PSAMP2Packet::~PSAMP2Packet() {
}

void PSAMP2Packet::onDataRecord(IpfixDataRecord* record) {
	recievedDataRecords++;
	boost::shared_ptr<IpfixRecord::TemplateInfo> dataTemplateInfo =
			record->templateInfo;
	/*check if it contains IP-Header+Payload*/
	IpfixRecord::FieldInfo* fi = dataTemplateInfo->getFieldInfo(
			PSAMP_TYPEID_ipHeaderPacketSection, 0);
	if (fi != 0) {
		msg(MSG_DEBUG, "PSAMP2Packet: IP-Header+Payload found");
		validPSAMP++;
		/*copy IP-Header+Payload to our temporary packet*/
		memcpy(packet + Packet::IPHeaderOffset, (record->data + fi->offset),
				record->dataLength);
		/*create packet with content of our temporary packet*/
		gettimeofday(&timestamp, NULL);
		Packet *p = packetManager.getNewInstance();
		p->init((char*) packet, Packet::IPHeaderOffset + record->dataLength,
				timestamp, 0, Packet::IPHeaderOffset + record->dataLength);
		send(p);
	} else {
		msg(MSG_DEBUG, "PSAMP2Packet: IP-Header+Payload not found");
	}
	/*remove reference from IpfixDataRecord*/
	record->removeReference();
}

string PSAMP2Packet::getStatisticsXML(double interval) {
	ostringstream oss;
	oss << "<recievedDataRecords>" << recievedDataRecords
			<< "</recievedDataRecords>";
	oss << "<validPSAMP>" << validPSAMP << "</validPSAMP>";
	return oss.str();
}
