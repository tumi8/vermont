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
