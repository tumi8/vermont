/*
 * VERMONT
 * Copyright (C) 2007 Tobias Limmer <tobias.limmer@informatik.uni-erlangen.de>
 * Copyright (C) 2014 Oliver Gasser
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

#include "Connection.h"
#include "common/crc.hpp"
#include "common/Misc.h"
#include "common/ipfixlolib/ipfix.h"

#include <sstream>
#include <algorithm>
#include <iostream>
#include <iomanip>


/**
 * creates new connection element
 * and initializes values with given IPFIX record
 * NOTE: all values are *copied*, no reference will be kept to original IPFIX record
 * @param connTimeout time in seconds when connection element times out
 */
Connection::Connection(IpfixDataRecord* record)
	: srcOctets(0), dstOctets(0),
	  srcTransOctets(0), dstTransOctets(0),
	  srcPackets(0), dstPackets(0),
	  srcTcpControlBits(0), dstTcpControlBits(0),
	  srcPayload(0), srcPayloadLen(0),
	  dstPayload(0), dstPayloadLen(0),
	  dpaForcedExport(0), dpaFlowCount(0),
	  dpaReverseStart(0)
{
	// convert IpfixDataRecord to Connection
	TemplateInfo::FieldInfo* fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_sourceIPv4Address, 0);
	if (fi != 0) {
		srcIP = *(uint32_t*)(record->data + fi->offset);
	} else {
		msg(LOG_NOTICE, "failed to determine source ip for record, assuming 0.0.0.0");
		srcIP = 0;
	}
	fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_destinationIPv4Address, 0);
	if (fi != 0) {
		dstIP = *(uint32_t*)(record->data + fi->offset);
	} else {
		msg(LOG_NOTICE, "failed to determine destination ip for record, assuming 0.0.0.0");
		dstIP = 0;
	}
	fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_sourceTransportPort, 0);
	if (fi != 0) {
		srcPort = *(uint16_t*)(record->data + fi->offset);
	} else {
		msg(LOG_NOTICE, "failed to determine source port for record, assuming 0");
		srcPort = 0;
	}
	fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_destinationTransportPort, 0);
	if (fi != 0) {
		dstPort = *(uint16_t*)(record->data + fi->offset);
	} else {
		msg(LOG_NOTICE, "failed to determine destination port for record, assuming 0");
		srcPort = 0;
	}
	fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_protocolIdentifier, 0);
	if (fi != 0) {
		protocol = *(uint8_t*)(record->data + fi->offset);
	} else {
		msg(LOG_NOTICE, "failed to determine protocol for record, using 0");
		protocol = 0;
	}

	fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_flowStartNanoseconds, 0);
	if (fi != 0) {
		convertNtp64(*(uint64_t*)(record->data + fi->offset), srcTimeStart);
	} else {
		fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_flowStartMilliseconds, 0);
		if (fi != 0) {
			srcTimeStart = ntohll(*(uint64_t*)(record->data + fi->offset));
		} else {
			fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_flowStartSeconds, 0);
			if (fi != 0) {
				srcTimeStart = ntohl(*(uint32_t*)(record->data + fi->offset));
				srcTimeStart *= 1000;
			} else {
				srcTimeStart = 0;
			}
		}
	}
	fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_flowEndNanoseconds, 0);
	if (fi != 0) {
		convertNtp64(*(uint64_t*)(record->data + fi->offset), srcTimeEnd);
	} else {
		fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_flowEndMilliseconds, 0);
		if (fi != 0) {
			srcTimeEnd = ntohll(*(uint64_t*)(record->data + fi->offset));
		} else {
			fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_flowEndSeconds, 0);
			if (fi != 0) {
				srcTimeEnd = ntohl(*(uint32_t*)(record->data + fi->offset));
				srcTimeEnd *= 1000;
			} else {
				srcTimeEnd = 0;
			}
		}
	}
	fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_flowStartNanoseconds, IPFIX_PEN_reverse);
	if (fi != 0) {
		convertNtp64(*(uint64_t*)(record->data + fi->offset), dstTimeStart);
	} else {
		fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_flowStartMilliseconds, IPFIX_PEN_reverse);
		if (fi != 0) {
			dstTimeStart = ntohll(*(uint64_t*)(record->data + fi->offset));
		} else {
			fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_flowStartSeconds, IPFIX_PEN_reverse);
			if (fi != 0) {
				dstTimeStart = ntohl(*(uint32_t*)(record->data + fi->offset));
				dstTimeStart *= 1000;
			} else {
				dstTimeStart = 0;
			}
		}
	}
	fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_flowEndNanoseconds, IPFIX_PEN_reverse);
	if (fi != 0) {
		convertNtp64(*(uint64_t*)(record->data + fi->offset), dstTimeEnd);
	} else {
		fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_flowEndMilliseconds, IPFIX_PEN_reverse);
		if (fi != 0) {
			dstTimeEnd = ntohll(*(uint64_t*)(record->data + fi->offset));
		} else {
			fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_flowEndSeconds, IPFIX_PEN_reverse);
			if (fi != 0) {
				dstTimeEnd = ntohl(*(uint32_t*)(record->data + fi->offset));
				dstTimeEnd *= 1000;
			} else {
				dstTimeEnd = 0;
			}
		}
	}
	fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_octetDeltaCount, 0);
	if (fi != 0) srcOctets = *(uint64_t*)(record->data + fi->offset);
	fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_octetDeltaCount, IPFIX_PEN_reverse);
	if (fi != 0) dstOctets = *(uint64_t*)(record->data + fi->offset);
	fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_packetDeltaCount, 0);
	if (fi != 0) srcPackets = *(uint64_t*)(record->data + fi->offset);
	fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_packetDeltaCount, IPFIX_PEN_reverse);
	if (fi != 0) dstPackets = *(uint64_t*)(record->data + fi->offset);
	fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_tcpControlBits, 0);
	if (fi != 0) {
		/*
		 * RFC rfc7011 and rfc7012 changed the tcpControlBits size
		 * from 1 byte to 2 bytes. Support both as the RFC mandates.
		 */
		if (fi->type.length == 2) {
			srcTcpControlBits = *(uint16_t*)(record->data + fi->offset) & Connection::MASK;
		} else if (fi->type.length == 1) {
			srcTcpControlBits = htons((uint16_t)*(uint8_t*)(record->data + fi->offset));
		} else {
			srcTcpControlBits = 0;
		}
	}
	fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_tcpControlBits, IPFIX_PEN_reverse);
	if (fi != 0) {
		/*
		 * RFC rfc7011 and rfc7012 changed the tcpControlBits size
		 * from 1 byte to 2 bytes. Support both as the RFC mandates.
		 */
		if (fi->type.length == 2) {
			dstTcpControlBits = *(uint16_t*)(record->data + fi->offset) & Connection::MASK;
		} else if (fi->type.length == 1) {
			dstTcpControlBits = htons((uint16_t)*(uint8_t*)(record->data + fi->offset));
		} else {
			dstTcpControlBits = 0;
		}
	}
	fi = record->templateInfo->getFieldInfo(IPFIX_ETYPEID_frontPayload, IPFIX_PEN_vermont);
	if (fi != 0 && fi->type.length) {
		TemplateInfo::FieldInfo* filen = record->templateInfo->getFieldInfo(IPFIX_ETYPEID_frontPayloadLen, IPFIX_PEN_vermont);
		if (filen != 0)
			srcPayloadLen = ntohl(*(uint32_t*)(record->data + filen->offset));
		else
			srcPayloadLen = fi->type.length;
		srcPayload = new char[srcPayloadLen];
		memcpy(srcPayload, record->data + fi->offset, srcPayloadLen);
	}
	fi = record->templateInfo->getFieldInfo(IPFIX_ETYPEID_frontPayload, IPFIX_PEN_vermont|IPFIX_PEN_reverse);
	if (fi != 0 && fi->type.length) {
		TemplateInfo::FieldInfo* filen = record->templateInfo->getFieldInfo(IPFIX_ETYPEID_frontPayloadLen, IPFIX_PEN_vermont|IPFIX_PEN_reverse);
		if (filen != 0)
			dstPayloadLen = ntohl(*(uint32_t*)(record->data + filen->offset));
		else
			dstPayloadLen = fi->type.length;
		dstPayload = new char[dstPayloadLen];
		memcpy(dstPayload, record->data + fi->offset, dstPayloadLen);
	}
	fi = record->templateInfo->getFieldInfo(IPFIX_ETYPEID_frontPayloadPktCount, IPFIX_PEN_vermont);
	if (fi != 0) srcPayloadPktCount= *(uint32_t*)(record->data + fi->offset);
	fi = record->templateInfo->getFieldInfo(IPFIX_ETYPEID_dpaForcedExport, IPFIX_PEN_vermont);
	if (fi != 0) dpaForcedExport = *(uint8_t*)(record->data + fi->offset);
	fi = record->templateInfo->getFieldInfo(IPFIX_ETYPEID_dpaReverseStart, IPFIX_PEN_vermont);
	if (fi != 0) dpaReverseStart = *(uint8_t*)(record->data + fi->offset);
	fi = record->templateInfo->getFieldInfo(IPFIX_ETYPEID_dpaFlowCount, IPFIX_PEN_vermont);
	if (fi != 0) dpaFlowCount = ntohl(*(uint32_t*)(record->data + fi->offset));
	fi = record->templateInfo->getFieldInfo(IPFIX_ETYPEID_transportOctetDeltaCount, IPFIX_PEN_vermont);
	if (fi != 0) srcTransOctets = ntohll(*(uint64_t*)(record->data + fi->offset));
	fi = record->templateInfo->getFieldInfo(IPFIX_ETYPEID_transportOctetDeltaCount, IPFIX_PEN_vermont|IPFIX_PEN_reverse);
	if (fi != 0) dstTransOctets = ntohll(*(uint64_t*)(record->data + fi->offset));
}

Connection::~Connection()
{
	if (srcPayload) delete[] srcPayload;
	if (dstPayload) delete[] dstPayload;
}

/**
 * swaps all data fields inside the connection
 */
void Connection::swapDataFields()
{
	swap(srcIP, dstIP);
	swap(srcPort, dstPort);
	swap(srcTimeStart, dstTimeStart);
	swap(srcTimeEnd, dstTimeEnd);
	swap(srcOctets, dstOctets);
	swap(srcPackets, dstPackets);
	swap(srcTcpControlBits, dstTcpControlBits);
	swap(srcPayload, dstPayload);
	swap(srcPayloadLen, dstPayloadLen);
}

/**
 * a nice little function here: it tries to determine the host which initiated the
 * connection and, if needed, swaps all data so that the initiating host is
 * specified as source host
 * @returns true if fields were swapped
 */
bool Connection::swapIfNeeded()
{
	// now try the starting time
	if ((dstTimeStart!=0) && (srcTimeStart>dstTimeStart)) {
		swapDataFields();
		return true;
	}
	return false;
}

string Connection::printTcpControlBits(uint16_t bits)
{
	ostringstream oss;
	const string strbits[] = { "", "", "", "", "", "", "", "NS",
			"CWR", "ECE", "URG", "ACK", "PSH", "RST", "SYN", "FIN" };
	for (int i=0; i<16; i++) {
		if ((bits&0x100)>0) oss << strbits[i];
		bits <<= 1;
	}
	return oss.str();
}


string Connection::toString()
{
	ostringstream oss;

	oss << "connection: " << endl;
	if (srcIP) oss << "srcIP: " << IPToString(srcIP) << endl;
	if (dstIP) oss << "dstIP: " << IPToString(dstIP) << endl;
	if (srcPort) oss << "srcPort: " << ntohs(srcPort) << endl;
	if (dstPort) oss << "dstPort: " << ntohs(dstPort) << endl;
	if (srcTimeStart) oss << "srcTimeStart: " << srcTimeStart << endl;
	if (srcTimeEnd) oss << "srcTimeEnd: " << srcTimeEnd << endl;
	if (dstTimeStart) oss << "dstTimeStart: " << dstTimeStart << endl;
	if (dstTimeEnd) oss << "dstTimeEnd: " << dstTimeEnd << endl;
	oss << "srcOctets: " << ntohll(srcOctets) << ", dstOctets: " << ntohll(dstOctets) << endl;
	oss << "srcPackets: " << ntohll(srcPackets) << ", dstPackets: " << ntohll(dstPackets) << endl;
	if (srcTcpControlBits || dstTcpControlBits) oss << "srcTcpControlBits: " << printTcpControlBits(srcTcpControlBits)
													<< ", dstTcpControlBits: " << printTcpControlBits(dstTcpControlBits) << endl;
	if (protocol) oss << "protocol: " << static_cast<uint32_t>(protocol) << endl;
	oss << "srcPayloadLen: " << srcPayloadLen << endl;
	oss << "dstPayloadLen: " << dstPayloadLen << endl;

	if (srcPayloadLen>0) {
		oss << "srcPayload: " << payloadToPlain(srcPayload, srcPayloadLen) << endl;
		oss << "srcPayload: " << payloadToHex(srcPayload, srcPayloadLen) << endl;
	}
 	oss << "dstPayloadLen: " << dstPayloadLen << endl;
	if (dstPayloadLen>0) {
		oss << "dstPayload: " << payloadToPlain(dstPayload, dstPayloadLen) << endl;
		oss << "dstPayload: " << payloadToHex(dstPayload, dstPayloadLen) << endl;
	}

	return oss.str();
}

/**
 * compares aggregatable fields to another connection
 * @param to connection direction to be regarded, true if src->dst
 */
bool Connection::compareTo(Connection* c, bool to)
{
	if (to) {
		return memcmp(&srcIP, &c->srcIP, 12)==0;
	} else {
		return (srcIP==c->dstIP && dstIP==c->srcIP &&
				srcPort==c->dstPort && dstPort==c->srcPort);
	}
}

/**
 * calculates hash from flow
 * @param which direction should be used? true if src->dst, else false
 * @param maxval maximum value to be returned - ATTENTION: only use values of 2**x-1!
 */
uint32_t Connection::getHash(bool to, uint32_t maxval)
{
	if (to) {
		return crc32(0, 12, reinterpret_cast<char*>(&srcIP)) & maxval;
	} else {
		uint16_t hash = crc32(0, 4, reinterpret_cast<char*>(&dstIP));
		hash = crc32(hash, 4, reinterpret_cast<char*>(&srcIP));
		hash = crc32(hash, 2, reinterpret_cast<char*>(&dstPort));
		hash = crc32(hash, 2, reinterpret_cast<char*>(&srcPort));
		return hash & maxval;
	}
}

/**
 * aggregates fields from given connection into this connection
 * @param inactiveExpireTime seconds until this connection expires
 * @param to true if this connection has to be aggregated in direction src->dst or false if reverse
 */
void Connection::aggregate(Connection* c, uint32_t inactiveExpireTime, bool to)
{
	timeExpire = time(0) + inactiveExpireTime;

	if (to) {
		srcOctets += c->srcOctets;
		srcPackets += c->srcPackets;
		srcTcpControlBits |= c->srcTcpControlBits;
		if (c->srcTimeStart < srcTimeStart) srcTimeStart = c->srcTimeStart;
		if (c->srcTimeEnd > srcTimeEnd) srcTimeEnd = c->srcTimeEnd;
	} else {
		dstOctets += c->srcOctets;
		dstPackets += c->srcPackets;
		dstTcpControlBits |= c->srcTcpControlBits;
		if (c->dstTimeStart < srcTimeStart) dstTimeStart = c->srcTimeStart;
		if (c->dstTimeEnd > srcTimeEnd) dstTimeEnd = c->srcTimeEnd;
	}
}

string Connection::payloadToPlain(const char* payload, uint32_t len)
{
	ostringstream oss;
	for (uint32_t i=0; i<len; i++) {
		if (isprint(payload[i])) oss << payload[i];
		else oss << '.';
	}
	return oss.str();
}

string Connection::payloadToHex(const char* payload, uint32_t len)
{
	ostringstream oss;
	char buf[4];
	for (uint32_t i=0; i<len; i++) {
		snprintf(buf, ARRAY_SIZE(buf), "%02hhX ", payload[i]);
		oss << buf;
	}
	return oss.str();
}
