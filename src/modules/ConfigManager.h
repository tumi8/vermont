/*
 * Vermont Configuration Subsystem
 * Copyright (C) 2009 Vermont Project
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

#ifndef CONFIGMANAGER_H_
#define CONFIGMANAGER_H_

/**
	@author Peter Baumann <siprbaum@users.berlios.de>
*/

#include "core/SensorManager.h"

#include "core/Cfg.h"
#include "core/DelayedDeleter.h"
#include "core/Graph.h"
#include "core/XMLDocument.h"
#include "core/GraphInstanceSupplier.h"

#include <string>

class ConfigManager
	: GraphInstanceSupplier
{
public:
	ConfigManager();

	virtual ~ConfigManager();
	
	void parseConfig(std::string fileName);

	void shutdown();
	Graph* getGraph();

private:
	Graph* graph;
	XMLDocument* document;
	XMLDocument* old_document;

	static Cfg* configModules[];
	
	DelayedDeleter deleter;
	SensorManager* sensorManager;
	
	
	void readGlobalConfig(XMLElement* e);
};

#endif /*CONFIGMANAGER_H_*/
