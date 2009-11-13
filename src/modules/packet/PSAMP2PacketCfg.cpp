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

#include "PSAMP2PacketCfg.h"

PSAMP2PacketCfg* PSAMP2PacketCfg::create(XMLElement* e) {
	assert(e);
	assert(e->getName() == getName());
	return new PSAMP2PacketCfg(e);
}

PSAMP2PacketCfg::PSAMP2PacketCfg(XMLElement* elem) :
	CfgHelper<PSAMP2Packet, PSAMP2PacketCfg> (elem, "psamp2packet") {
	if (!elem)
		return;
}

PSAMP2PacketCfg::~PSAMP2PacketCfg() {
}

PSAMP2Packet* PSAMP2PacketCfg::createInstance() {
	instance = new PSAMP2Packet();
	return instance;
}

bool PSAMP2PacketCfg::deriveFrom(PSAMP2PacketCfg* old) {
	return false;
}
