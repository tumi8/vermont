#ifdef HAVE_CONNECTION_FILTER

#include "ConnectionFilter.h"

ConnectionFilter::ConnectionFilter(unsigned Timeout, unsigned bytes, unsigned hashFunctions, unsigned filterSize)
	:  hashParams(hashFunctions), synFilter(&hashParams, filterSize, false), exportFilter(&hashParams, filterSize, false),
	  connectionFilter(&hashParams, filterSize, false), timeout(Timeout), exportBytes(bytes)
{
}

ConnectionFilter::ConnectionFilter(unsigned Timeout, unsigned bytes, unsigned hashFunctions, unsigned filterSize, unsigned seed)
	: hashParams(hashFunctions, seed), synFilter(&hashParams, filterSize, false), exportFilter(&hashParams, filterSize, false),
	connectionFilter(&hashParams, filterSize, false), timeout(Timeout), exportBytes(bytes)
{
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

	if (*((uint8_t*)p->data + flagsOffset) & SYN) {
		DPRINTF("ConnectionFilter: Got SYN packet");
		synFilter.set(key.data, key.len, (agetime_t)p->timestamp.tv_sec);
		DPRINTF("ConnectionFilter: synFilter saved time %u", synFilter.get(key.data, key.len));
		// do not export SYN packet, or should we?
		return false;
	} else if (*((uint8_t*)p->data + flagsOffset) & RST || *((uint8_t*)p->data + flagsOffset) & FIN) {
		
		DPRINTF("ConnectionFilter: Got %s packet", *((uint8_t*)p->data + flagsOffset) & RST?"RST":"FIN");
	
		exportFilter.set(key.data, key.len, -exportFilter.get(key.data, key.len));
		connectionFilter.set(key.data, key.len, p->timestamp.tv_sec);
		DPRINTF("ConnectionFilter: connectionFilter saved time %u", connectionFilter.get(key.data, key.len));

		// do not export FIN/RST packets, or should we?
		return false;
	} else {
		DPRINTF("ConnectionFilter: Got a normal packet");
		if ((tmp = exportFilter.get(key.data, key.len)) > 0) {
			DPRINTF("ConnectionFilter: Connection known, exporting packet");
			static unsigned diffVal;
			if (tmp > payloadLen)
				diffVal = -payloadLen;
			else
				diffVal = -tmp;
			exportFilter.set(key.data, key.len, diffVal);
			if (exportFilter.get(key.data, key.len) <= 0) {
				connectionFilter.set(key.data, key.len, p->timestamp.tv_sec);
			}
			DPRINTF("ConnectionFilter: We have to export %i bytes after exporting this packet", exportFilter.get(key.data, key.len));
			return true;
		} else {
			if ((unsigned)(p->timestamp.tv_sec - synFilter.get(key.data, key.len)) < timeout &&
			    synFilter.get(key.data, key.len) - connectionFilter.get(key.data, key.len) > 0) {
			    	DPRINTF("ConnectionFilter: Found new connection, exporting packet");
				if (payloadLen < exportBytes) {
					exportFilter.set(key.data, key.len, exportBytes - payloadLen);
				} else {
					connectionFilter.set(key.data, key.len, p->timestamp.tv_sec);
				}
				DPRINTF("ConnectionFilter: We have to export %i bytes after exporting this packet", exportFilter.get(key.data, key.len));
				return true;
			}
			DPRINTF("ConnectionFilter: Paket will not be exported");
			return false;
		}
	}

	msg(MSG_FATAL, "ConnectionFilter: SOMTHING IS SCRWED UP, YOU SHOULD NEVER SEE THIS MESSAGE!");
	return false; // make compiler happy
}

#endif
