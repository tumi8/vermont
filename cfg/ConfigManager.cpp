#include "ConfigManager.h"

#include "cfg/ObserverCfg.h"
#include "cfg/PacketFilterCfg.h"
#include "cfg/PacketPrinterCfg.h"
#include "cfg/QueueCfg.h"


Cfg* ConfigManager::configModules[] = {
	new ObserverCfg(NULL),
	new PacketFilterCfg(NULL),
	new PacketQueueCfg(NULL),
	new PacketPrinterCfg(NULL),
};
		
ConfigManager::ConfigManager()
{
	graph = new Graph();
}

ConfigManager::~ConfigManager()
{
	delete graph;
}

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))


/*void addNodeAndMapping(Graph* g, Cfg* cfg)
{

	std::map<int, std::pair<Node*, Cfg*> >::iterator it = m.find(cfg->getID());
	if (it != m.end())
		THROWEXCEPTION("Duplicate node id\n");

	m[cfg->getID()] = NodeCfgPair(n, cfg);
}
*/

void ConfigManager::parseConfig(std::string fileName)
{
	Observer* observer = NULL;
	
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

				// FIXME: observer needs special hanlding
				if ((*it)->getName() == "observer")
					observer = dynamic_cast<ObserverCfg*>(cfg)->getInstance(); 
			}
		}
	}
	
	Connector connector;
	graph->accept(&connector);
	
	observer->startCapture();
}

template <class T>
static void parseSection(ConfigManager& manager, XMLElement* e)
{
	T* cfg = new T(e);
}
