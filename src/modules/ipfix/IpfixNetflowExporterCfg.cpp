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
#include "IpfixNetflowExporterCfg.h"

IpfixNetflowExporterCfg::IpfixNetflowExporterCfg(XMLElement* elem)
	: CfgHelper<IpfixNetflowExporter, IpfixNetflowExporterCfg>(elem, "ipfixNetflowExporter")
{
	if (!elem)
		return;

	destHost = get("host");
	destPort = getInt("port") & 0xFFFF;
	maxRecordRate = getInt("maxRecordRate", INE_DEFAULT_MAXRECORDRATE);
}

IpfixNetflowExporterCfg::~IpfixNetflowExporterCfg()
{
}

IpfixNetflowExporterCfg* IpfixNetflowExporterCfg::create(XMLElement* e)
{
	return new IpfixNetflowExporterCfg(e);
}

IpfixNetflowExporter* IpfixNetflowExporterCfg::createInstance()
{
	instance = new IpfixNetflowExporter(destHost, destPort, maxRecordRate);
	return instance;
}

bool IpfixNetflowExporterCfg::deriveFrom(IpfixNetflowExporterCfg* old)
{
	return true;
}

