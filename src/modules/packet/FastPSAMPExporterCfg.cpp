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

#include "modules/packet/PacketReportingCfg.h"
#include "modules/packet/FastPSAMPExporterCfg.h"
#include "modules/packet/FastPSAMPExporterModule.h"

#include "common/defs.h"

#include <cassert>

FastPSAMPExporterCfg::FastPSAMPExporterCfg(XMLElement* elem) :
	CfgHelper<FastPSAMPExporterModule, FastPSAMPExporterCfg> (elem,
			"fastpsampExporter") {
	if (!elem)
		return;
}

FastPSAMPExporterCfg* FastPSAMPExporterCfg::create(XMLElement* elem) {
	assert(elem);
	assert(elem->getName() == getName());
	return new FastPSAMPExporterCfg(elem);
}

FastPSAMPExporterCfg::~FastPSAMPExporterCfg() {
}

FastPSAMPExporterModule* FastPSAMPExporterCfg::createInstance() {
	instance = new FastPSAMPExporterModule("127.0.0.1", 4739);
	return instance;
}

bool FastPSAMPExporterCfg::deriveFrom(FastPSAMPExporterCfg* old) {
	return false;
}
