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

#ifndef IPFIX_FLOW_INSPECTOR_EXPORTER_CFG_H_
#define IPFIX_FLOW_INSPECTOR_EXPORTER_CFG_H_

#ifdef REDIS_SUPPORT_ENABLED

#include <core/XMLElement.h>
#include <core/Cfg.h>

#include "IpfixFlowInspectorExporter.hpp"

#include <string>

class IpfixFlowInspectorExporterCfg
	: public CfgHelper<IpfixFlowInspectorExporter, IpfixFlowInspectorExporterCfg>
{
public:
	friend class ConfigManager;
	
	virtual IpfixFlowInspectorExporterCfg* create(XMLElement* e);
	virtual ~IpfixFlowInspectorExporterCfg();
	
	virtual IpfixFlowInspectorExporter* createInstance();
	virtual bool deriveFrom(IpfixFlowInspectorExporterCfg* old);
	
protected:
	
	std::string hostname; /**< hostname of database host */
	uint16_t port;	/**< port of database */
	std::string database; /**< mongo database name */
	
	IpfixFlowInspectorExporterCfg(XMLElement*);
};


#endif /*REDIS_SUPPORT_ENABLED*/

#endif /*IPFIX_FLOW_INSPECTOR_EXPORTER_CFG_H_*/
