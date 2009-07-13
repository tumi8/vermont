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

#ifndef HOSTFILTER_H_
#define HOSTFILTER_H_

#include <set>
#include "PacketProcessor.h"

class HostFilter : public PacketProcessor
{
public:
	HostFilter(std::string addrfilter, std::set<uint32_t> iplist);

	bool processPacket(Packet *p);

private:
	std::string addrFilter;
	std::set<uint32_t> ipList;
};

#endif /*HOSTFILTER_H_*/
