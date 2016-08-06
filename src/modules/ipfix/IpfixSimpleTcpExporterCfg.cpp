/*
 * IPFIX Netflow Exporter
 * Copyright (C) 20101 Mario Volke
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
#include "IpfixSimpleTcpExporterCfg.h"

IpfixSimpleTcpExporterCfg::IpfixSimpleTcpExporterCfg(XMLElement* elem)
	: CfgHelper<IpfixSimpleTcpExporter, IpfixSimpleTcpExporterCfg>(elem, "ipfixSimpleTcpExporter")
{
	if (!elem)
		return;

	destHost = get("host");
	destPort = getInt("port") & 0xFFFF;
}

IpfixSimpleTcpExporterCfg::~IpfixSimpleTcpExporterCfg()
{
}

IpfixSimpleTcpExporterCfg* IpfixSimpleTcpExporterCfg::create(XMLElement* e)
{
	return new IpfixSimpleTcpExporterCfg(e);
}

IpfixSimpleTcpExporter* IpfixSimpleTcpExporterCfg::createInstance()
{
	instance = new IpfixSimpleTcpExporter(destHost, destPort);
	return instance;
}

bool IpfixSimpleTcpExporterCfg::deriveFrom(IpfixSimpleTcpExporterCfg* old)
{
	return false;
}

