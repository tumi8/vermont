/*
 * IPFIX Netflow Exporter
 * Copyright (C) 2010 Tobias Limmer
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
#ifndef IPFIXNETFLOWEXPORTERCFG_H_
#define IPFIXNETFLOWEXPORTERCFG_H_

#include "core/Cfg.h"
#include "IpfixNetflowExporter.h"

class IpfixNetflowExporterCfg
	: public CfgHelper<IpfixNetflowExporter, IpfixNetflowExporterCfg>
{
public:
	friend class ConfigManager;

	virtual ~IpfixNetflowExporterCfg();

	virtual IpfixNetflowExporterCfg* create(XMLElement* e);

	virtual IpfixNetflowExporter* createInstance();

	virtual bool deriveFrom(IpfixNetflowExporterCfg* old);

protected:
	IpfixNetflowExporterCfg(XMLElement*);

private:
	string destHost;
	uint16_t destPort;
	uint32_t maxRecordRate;
};

#endif
