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

#ifndef PSAMP2PACKETCFG_H_
#define PSAMP2PACKETCFG_H_

#include <core/XMLElement.h>

#include <core/Cfg.h>

#include "modules/packet/PSAMP2Packet.h"

#include <string>

using namespace std;

class PSAMP2PacketCfg: public CfgHelper<PSAMP2Packet, PSAMP2PacketCfg> {
	friend class ConfigManager;
public:
	virtual ~PSAMP2PacketCfg();

	virtual PSAMP2PacketCfg* create(XMLElement* elem);

	virtual PSAMP2Packet* createInstance();

	bool deriveFrom(PSAMP2PacketCfg* old);

protected:
	PSAMP2PacketCfg(XMLElement* elem);
};

#endif
