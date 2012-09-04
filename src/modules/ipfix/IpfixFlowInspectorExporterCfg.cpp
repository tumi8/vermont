/*
 * Vermont Configuration Subsystem
 * Copyright (C) 2012 Vermont Project
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

#ifdef REDIS_SUPPORT_ENABLED

#include "IpfixFlowInspectorExporterCfg.h"
#include <cassert>


IpfixFlowInspectorExporterCfg* IpfixFlowInspectorExporterCfg::create(XMLElement* e)
{
	assert(e);
	assert(e->getName() == getName());
	return new IpfixFlowInspectorExporterCfg(e);
}


IpfixFlowInspectorExporterCfg::IpfixFlowInspectorExporterCfg(XMLElement* elem)
	: CfgHelper<IpfixFlowInspectorExporter, IpfixFlowInspectorExporterCfg>(elem, "ipfixFlowInspectorExporter"),
		port(6379)
{
	if (!elem) return;

	XMLNode::XMLSet<XMLElement*> set = _elem->getElementChildren();
	for ( XMLNode::XMLSet<XMLElement*>::iterator it = set.begin(); it != set.end(); it++) {
		XMLElement* e = *it;
		if (e->matches("host")) {
			hostname = e->getFirstText();
		} else if (e->matches("port")) {
			port = getInt("port");
		} else if (e->matches("dbname")) {
			database = e->getFirstText();
		} else if (e->matches("next")) { // ignore next
		} else {
			msg(MSG_FATAL, "Unknown IpfixFlowInspectorExporter config statement %s\n", e->getName().c_str());
			continue;
		}
	}
	if (hostname=="") THROWEXCEPTION("IpfixFlowInspectorExporterCfg: host not set in configuration!");
	if (database=="") THROWEXCEPTION("IpfixFlowInspectorExporterCfg: dbname not set in configuration!");
}

IpfixFlowInspectorExporterCfg::~IpfixFlowInspectorExporterCfg()
{
}


IpfixFlowInspectorExporter* IpfixFlowInspectorExporterCfg::createInstance()
{
	instance = new IpfixFlowInspectorExporter(hostname, database, port);
	msg(MSG_DEBUG, "IpfixFlowInspectorExporter configuration host %s queue %s port %i\n", hostname.c_str(), database.c_str(), port);
	return instance;
}


bool IpfixFlowInspectorExporterCfg::deriveFrom(IpfixFlowInspectorExporterCfg* old)
{
	return false;
}

#endif /*REDIS_SUPPORT_ENABLED*/
