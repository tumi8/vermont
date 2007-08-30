#ifndef CONFIGMANAGER_H_
#define CONFIGMANAGER_H_

/**
	@author Peter Baumann <siprbaum@users.berlios.de>
*/

#include "cfg/Cfg.h"
#include "cfg/Graph.h"
#include "cfg/XMLDocument.h"

#include <string>

class ConfigManager
{
public:
	ConfigManager() : graph(NULL) {	}
	virtual ~ConfigManager();
	
	void parseConfig(std::string fileName);

private:
	Graph* graph;
	XMLDocument* document;

	static Cfg* configModules[];
};

#endif /*CONFIGMANAGER_H_*/
