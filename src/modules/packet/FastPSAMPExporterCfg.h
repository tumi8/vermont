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

#ifndef FastPSAMPEXPORTERCFG_H_
#define FastPSAMPEXPORTERCFG_H_

#include "core/Cfg.h"
#include "modules/ipfix/CollectorCfg.h"
#include "modules/packet/FastPSAMPExporterModule.h"

#include <vector>

// forward declarations
class PacketReportingCfg;

class FastPSAMPExporterCfg: public CfgHelper<FastPSAMPExporterModule,
		FastPSAMPExporterCfg> {
	friend class ConfigManager;
public:
	virtual ~FastPSAMPExporterCfg();

	virtual FastPSAMPExporterCfg* create(XMLElement* elem);

	virtual FastPSAMPExporterModule* createInstance();

	bool deriveFrom(FastPSAMPExporterCfg* old);

protected:
	FastPSAMPExporterCfg(XMLElement* elem);

private:

};

#endif /*PSAMPEXPORTERCFG_H_*/
