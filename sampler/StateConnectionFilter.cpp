#include "StateConnectionFilter.h"

#include <iostream>

StateConnectionFilter::StateConnectionFilter(unsigned timeout, unsigned bytes)
{
	this->timeout = timeout;
	this->exportBytes = bytes;
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

	if (*((uint8_t*)p->data + flagsOffset) & SYN) {
		DPRINTF("StateConnectionFilter: Got SYN packet");
		exportList[key] = exportBytes;
		return false;
	} else if (*((uint8_t*)p->data + flagsOffset) & RST || *((uint8_t*)p->data + flagsOffset) & FIN) {
		DPRINTF("StateConnectionFilter: Got %s packet", *((uint8_t*)p->data + flagsOffset) & RST?"RST":"FIN");
		if (exportList.find(key) != exportList.end()) {
			exportList.erase(exportList.find(key));
		}
		return false;
	} else {
		DPRINTF("StateConnectionFilter: Got a normal packet");
		if (exportList.find(key) != exportList.end() && exportList.find(key)->second > 0) {
			DPRINTF("StateConnectionFilter: Connection known, exporting packet");
			exportList[key] -= payloadLen;
			DPRINTF("StateConnectionFilter: We have to export %i bytes after exporting this packet", exportList[key]>0?exportList[key]:0);
			return true;
		} else {
			if (exportList.find(key) != exportList.end()) {
				exportList.erase(exportList.find(key));
			}
			return false;
		}
	}

	msg(MSG_FATAL, "StateConnectionFilter: SOMTHING IS SCRWED UP, YOU SHOULD NEVER SEE THIS MESSAGE!");
	return false; // make compiler happy
}


