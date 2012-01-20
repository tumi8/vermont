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
#include "core/Graph.h"
#include "core/XMLDocument.h"
#include "core/GraphInstanceSupplier.h"

#include <string>
#include <ctime>
typedef struct deleter_list_item {
	Cfg* c;
	time_t delete_after;
} deleter_list_item;

class ConfigManager
	: GraphInstanceSupplier
{
	friend class Reconnector;
public:
	ConfigManager();

	virtual ~ConfigManager();
	
	void parseConfig(std::string fileName);

	void onTimeout2();	
	void shutdown();
	Graph* getGraph();
	Graph* reconnect(Graph *g, Graph *o);

private:
	Graph* graph;
	XMLDocument* document;
	XMLDocument* old_document;

	static Cfg* configModules[];
	
	SensorManager* sensorManager;
	
	void readGlobalConfig(XMLElement* e);
	std::list<deleter_list_item> deleter_list;
};

#endif /*CONFIGMANAGER_H_*/
