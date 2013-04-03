/*
 * VERMONT
 * Copyright (C) 2009 Matthias Segschneider <matthias.segschneider@gmx.de>
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

#include "HostFilter.h"
#include "common/Misc.h"

#define IPV4_ADDRESS_BYTE_LENGTH	4		// Bytes
#define IPV4_SRC_IP_OFFSET			12
#define IPV4_DST_IP_OFFSET			16


HostFilter::HostFilter(std::string addrFilter, std::set<uint32_t> ipList)
	: addrFilter(addrFilter), ipList(ipList)
{
}


bool HostFilter::processPacket(Packet *p)
{
	// if no IPv4 Packet
	if ((p->classification & PCLASS_NET_IP4)==0)
	{
		msg(MSG_DIALOG, "false?");
		return false;
	}
	// srcIP
	uint32_t srcIp = *reinterpret_cast<uint32_t*>(p->data.netHeader+IPV4_SRC_IP_OFFSET);
	msg(MSG_DIALOG, "srcip: %X, %s", srcIp, IPToString(srcIp).c_str());
	// dstIP
	uint32_t dstIp = *reinterpret_cast<uint32_t*>(p->data.netHeader+IPV4_DST_IP_OFFSET);

	if (addrFilter == "src") {
		return (ipList.find(srcIp) != ipList.end());
	} else if (addrFilter == "dst") {
		return (ipList.find(dstIp) != ipList.end());
	} else if (addrFilter == "both") {
		return ((ipList.find(srcIp) != ipList.end()) || (ipList.find(dstIp) != ipList.end()));
	} else {
		msg(MSG_FATAL, "Unknown hostFilter config statement %s\n", addrFilter.c_str());
		return false;
	}
}
