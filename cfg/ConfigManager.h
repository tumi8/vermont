#ifndef CONFIGMANAGER_H_
#define CONFIGMANAGER_H_

/**
	@author Peter Baumann <siprbaum@users.berlios.de>
*/

#include "cfg/Cfg.h"
#include "cfg/DelayedDeleter.h"
#include "cfg/Graph.h"
#include "cfg/XMLDocument.h"

#include <string>

class ConfigManager
{
public:
	ConfigManager() : graph(NULL), document(NULL), old_document(NULL) { }

	virtual ~ConfigManager();
	
	void parseConfig(std::string fileName);

	void shutdown();

private:
	Graph* graph;
	XMLDocument* document;
	XMLDocument* old_document;

	static Cfg* configModules[];
	
	DelayedDeleter deleter;
};

#endif /*CONFIGMANAGER_H_*/
