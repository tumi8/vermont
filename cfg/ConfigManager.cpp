#include "ConfigManager.h"
#include "cfg/Connector.h"

#include "cfg/ObserverCfg.h"
#include "cfg/PacketFilterCfg.h"
#include "cfg/PacketPrinterCfg.h"
#include "cfg/QueueCfg.h"

#include "common/PacketInstanceManager.h"

#include <cassert>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

Cfg* ConfigManager::configModules[] = {
	new ObserverCfg(NULL),
	new PacketFilterCfg(NULL),
	new PacketQueueCfg(NULL),
	new PacketPrinterCfg(NULL),
};

ConfigManager::~ConfigManager()
{
	if (graph) {
		vector<CfgNode*> nodes = graph->getNodes();
		for (size_t i = 0; i < nodes.size(); i++) {
			delete nodes[i]->getCfg();
		}
		delete graph;
	}

	PacketInstanceManager::destroyManager();

	for (size_t i = 0; i < ARRAY_SIZE(configModules); i++) {
		delete configModules[i];
	}
}

void ConfigManager::parseConfig(std::string fileName)
{
	Observer* observer = NULL;
	Graph* oldGraph = graph;

	graph = new Graph();

	old_document = document;
	document = XMLDocument::parse_file(fileName);
	XMLElement* root = document->getRootNode();

	// consistency checks
	if (!root)
		THROWEXCEPTION("%s is an empty XML-Document!", fileName.c_str());

	if (!root->matches("ipfixConfig"))
		THROWEXCEPTION("Root element does not match \"ipfixConfig\"."
			       " This is not a valid configuration file!");

	/* process each root element node and add a new node (with its config 
	 * attached to the node) to the graph
	 */
	XMLNode::XMLSet<XMLElement*> rootElements = root->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::const_iterator it = rootElements.begin();
	     it != rootElements.end();
	     it++) {
		for (unsigned int i = 0; i < ARRAY_SIZE(configModules); i++) {
			if ((*it)->getName() == configModules[i]->getName()) {
				Cfg* cfg = configModules[i]->create(*it);
				graph->addNode(cfg);

				// FIXME: observer needs special handling
				if ((*it)->getName() == "observer")
					observer = dynamic_cast<ObserverCfg*>(cfg)->getInstance();
			}
		}
	}

	if (!oldGraph) { // this is the first config we have read
		msg(MSG_FATAL, "---------- initialisiz");
		Connector connector;

		graph->accept(&connector);
		observer->startCapture(); // FIXME: observer could be reinstantiated
	} else {
		msg(MSG_FATAL, "---------- reconnect");
		// first, connect the nodes on the new graph (but NOT the modules
		Connector connector(true, false);
		graph->accept(&connector);
		msg(MSG_FATAL, "%s:%d", __FILE__, __LINE__);
		// now connect the modules reusing those from the old graph
		ReConnector reconnector(oldGraph);
		graph->accept(&reconnector);
	}

	if (old_document)
		delete old_document;
}

void ConfigManager::shutdown()
{
	std::vector<CfgNode*> topoNodes = graph->topoSort();

	for (size_t i = 0; i < topoNodes.size(); i++) {
		CfgNode* n = topoNodes[i];

		Cfg* cfg = n->getCfg();

		// shutdown the thread
		cfg->getInstance()->shutdown();

		// disconnect the module from its sources ..
		vector<CfgNode*> sources = graph->getSources(n);
		for (size_t k = 0; k < sources.size(); k++) {
			sources[k]->getCfg()->getInstance()->disconnect();
		}
	}

	// free memory
	// FIXME: this is racy because we could delete a destination which
	//        is sill used as the thread hasn't shutdown yet
	for (size_t i = 0; i < topoNodes.size(); i++) {
		CfgNode* n = topoNodes[i];

		DPRINTF("deleteting %s ..", n->getCfg()->getName().c_str());
		delete n->getCfg()->getInstance();
		delete n->getCfg();

		graph->removeNode(n);
		delete n;
	}
}
