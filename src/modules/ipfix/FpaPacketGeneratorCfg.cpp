/*
 * FPA PCAP Exporter
 * Copyright (C) 2009 Tobias Limmer <http://www.cs.fau.de/~limmer>
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
#include "FpaPacketGeneratorCfg.h"

#include "common/defs.h"

#include <cassert>
#include <pcap.h>


FpaPacketGeneratorCfg::FpaPacketGeneratorCfg(XMLElement* elem)
	: CfgHelper<FpaPacketGenerator, FpaPacketGeneratorCfg>(elem, "fpaPacketGenerator")
{
	if (!elem) return;

	// we do not have any configuration options
}

FpaPacketGeneratorCfg* FpaPacketGeneratorCfg::create(XMLElement* elem)
{
	assert(elem);
	assert(elem->getName() == getName());
	return new FpaPacketGeneratorCfg(elem);
}

FpaPacketGeneratorCfg::~FpaPacketGeneratorCfg()
{
}

FpaPacketGenerator* FpaPacketGeneratorCfg::createInstance()
{
	instance = new FpaPacketGenerator();
	return instance;
}

bool FpaPacketGeneratorCfg::deriveFrom(FpaPacketGeneratorCfg* old)
{
	return false; // FIXME: implement
}
