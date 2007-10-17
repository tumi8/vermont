#include "ConfigManager.h"
#include "cfg/Connector.h"

#include "cfg/CfgNode.h"
#include "cfg/IpfixAggregatorCfg.h"
#include "cfg/IpfixExporterCfg.h"
#include "cfg/ObserverCfg.h"
#include "cfg/PacketFilterCfg.h"
#include "cfg/PacketPrinterCfg.h"
#include "cfg/PSAMPExporterCfg.h"
#include "cfg/QueueCfg.h"

#include <cassert>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

// we create a static array of all root config entrys so that we don't
// need to hardcode the config entry name in here. Instead, we just ask the
// module instances if they handle the specific entry. 
Cfg* ConfigManager::configModules[] = {
	new ObserverCfg(NULL),
	new PacketFilterCfg(NULL),
	new PacketQueueCfg(NULL),
	new PacketPrinterCfg(NULL),
	new PSAMPExporterCfg(NULL),
	new IpfixExporterCfg(NULL),
	new IpfixAggregatorCfg(NULL),
};

ConfigManager::~ConfigManager()
{
	if (graph) {
		delete graph;
	}
}

void ConfigManager::parseConfig(std::string fileName)
{
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
			}
		}
	}

	if (!oldGraph) { // this is the first config we have read
		Connector connector;
		graph->accept(&connector);
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

	// start the instances if not already running
	std::vector<CfgNode*> topoNodes = graph->topoSort();

	for (size_t i = 0; i < topoNodes.size(); i++) {
		topoNodes[topoNodes.size() -1 -i]->getCfg()->start(false);
	}

	if (old_document)
		delete old_document;

	if (oldGraph)
		delete oldGraph;
}

void ConfigManager::shutdown()
{
	std::vector<CfgNode*> topoNodes = graph->topoSort();

	// shutdown the thread
	for (size_t i = 0; i < topoNodes.size(); i++) {
		Cfg* cfg = topoNodes[i]->getCfg();

		msg(MSG_FATAL, "start shuting down %s", cfg->getName().c_str());
		cfg->shutdown(false);
		msg(MSG_FATAL, "end shuting down %s", cfg->getName().c_str());
	}

	// disconnect the modules
	for (size_t i = 0; i < topoNodes.size(); i++) {
		CfgNode* n = topoNodes[i];
		Cfg* cfg = n->getCfg();

		// disconnect the module from its sources ..
		vector<CfgNode*> sources = graph->getSources(n);
		for (size_t k = 0; k < sources.size(); k++) {
			msg(MSG_FATAL, "run %s->disconnect", cfg->getName().c_str());
			sources[k]->getCfg()->disconnectInstances();
		}
	}
}
