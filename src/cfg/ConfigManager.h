#ifndef CONFIGMANAGER_H_
#define CONFIGMANAGER_H_

/**
	@author Peter Baumann <siprbaum@users.berlios.de>
*/

#include "SensorManager.h"

#include "cfg/Cfg.h"
#include "cfg/DelayedDeleter.h"
#include "cfg/Graph.h"
#include "cfg/XMLDocument.h"
#include "GraphInstanceSupplier.h"

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
