/*
 * IPFIX Netflow Exporter
 * Copyright (C) 2011 Mario Volke
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
#ifndef IPFIXSIMPLETCPEXPORTERCFG_H_
#define IPFIXSIMPLETCPEXPORTERCFG_H_

#include "core/Cfg.h"
#include "IpfixSimpleTcpExporter.h"

class IpfixSimpleTcpExporterCfg
	: public CfgHelper<IpfixSimpleTcpExporter, IpfixSimpleTcpExporterCfg>
{
public:
	friend class ConfigManager;

	virtual ~IpfixSimpleTcpExporterCfg();

	virtual IpfixSimpleTcpExporterCfg* create(XMLElement* e);

	virtual IpfixSimpleTcpExporter* createInstance();

	virtual bool deriveFrom(IpfixSimpleTcpExporterCfg* old);

protected:
	IpfixSimpleTcpExporterCfg(XMLElement*);

private:
	string destHost;
	uint16_t destPort;
};

#endif
