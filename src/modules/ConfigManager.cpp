#include "modules/ConfigManager.h"
#include "core/Connector.h"
#include "core/ReConnector.h"
#include "core/CfgNode.h"

#include "QueueCfg.h"
#include "AnonymizerCfg.h"
#include "modules/packet/ObserverCfg.h"
#include "modules/packet/PSAMPExporterCfg.h"
#include "modules/packet/PCAPExporterCfg.h"
#include "modules/packet/filter/PacketFilterCfg.h"
#include "modules/ipfix/FpaPcapExporterCfg.h"
#include "modules/ipfix/IpfixCollectorCfg.h"
#include "modules/ipfix/IpfixExporterCfg.h"
#include "modules/ipfix/IpfixPrinterCfg.h"
#include "modules/ipfix/IpfixDbReaderCfg.h"
#include "modules/ipfix/IpfixDbWriterCfg.h"
#include "modules/ipfix/IpfixFileWriterCfg.hpp"
#include "modules/ipfix/IpfixReceiverFileCfg.h"
#include "modules/ipfix/IpfixDbWriterPgCfg.h"
#include "modules/ipfix/IpfixPayloadWriterCfg.h"
#include "modules/ipfix/IpfixSamplerCfg.h"
#include "modules/ipfix/NetflowV9ConverterCfg.hpp"
#include "modules/ipfix/aggregator/IpfixAggregatorCfg.h"
#include "modules/ipfix/aggregator/PacketAggregatorCfg.h"
#include "modules/SensorManagerCfg.h"
#include "modules/analysis/TRWPortscanDetectorCfg.h"
#include "modules/analysis/RBSWormDetectorCfg.h"
#include "modules/analysis/FrontPayloadSigMatcherCfg.h"
#include "modules/analysis/AutoFocusCfg.h"
#include "modules/idmef/IDMEFExporterCfg.h"
#include "modules/idmef//PacketIDMEFReporterCfg.h"
#include "modules/analysis/P2PDetectorCfg.h"
#include "modules/analysis/HostStatisticsCfg.h"

#include <cassert>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

// we create a static array of all root config entrys so that we don't
// need to hardcode the config entry name in here. Instead, we just ask the
// module instances if they handle the specific entry.
Cfg* ConfigManager::configModules[] = {
	new ObserverCfg(NULL),
	new PacketFilterCfg(NULL),
	new PacketQueueCfg(NULL),
	new PCAPExporterCfg(NULL),
	new PSAMPExporterCfg(NULL),
	new FpaPcapExporterCfg(NULL),
	new IpfixCollectorCfg(NULL),
	new IpfixQueueCfg(NULL),
	new IpfixExporterCfg(NULL),
	new IpfixAggregatorCfg(NULL),
	new IpfixPrinterCfg(NULL),
	new IpfixSamplerCfg(NULL),
	new NetflowV9ConverterCfg(NULL),
	new PacketAggregatorCfg(NULL),
	new SensorManagerCfg(NULL),
	new TRWPortscanDetectorCfg(NULL),
	new RBSWormDetectorCfg(NULL),
	new AutoFocusCfg(NULL),
	new IDMEFExporterCfg(NULL),
	new PacketIDMEFReporterCfg(NULL),
	new IpfixReceiverFileCfg(NULL),
	new IpfixPayloadWriterCfg(NULL),
	new IpfixFileWriterCfg(NULL),
	new AnonymizerCfg(NULL),
	new FrontPayloadSigMatcherCfg(NULL),
	new P2PDetectorCfg(NULL),
	new HostStatisticsCfg(NULL),
#ifdef DB_SUPPORT_ENABLED
	new IpfixDbReaderCfg(NULL),
	new IpfixDbWriterCfg(NULL),
#endif
#ifdef PG_SUPPORT_ENABLED
	new IpfixDbWriterPgCfg(NULL),
#endif
};

ConfigManager::ConfigManager()
	: graph(NULL), document(NULL), old_document(NULL), sensorManager(NULL)
{

}

ConfigManager::~ConfigManager()
{
	if (graph) {
		delete graph;
	}
}

/**
 * parses configuration and adjusts/creates module graph accordingly
 * afterwards all modules are started
 */
void ConfigManager::parseConfig(std::string fileName)
{
	lockGraph();
	Graph* oldGraph = graph;

	graph = new Graph();

	old_document = document;
	document = XMLDocument::parse_file(fileName);
	XMLElement* root = document->getRootNode();

	// consistency checks
	if (!root) {
		unlockGraph();
		THROWEXCEPTION("%s is an empty XML-Document!", fileName.c_str());
	}

	if (!root->matches("ipfixConfig")) {
		unlockGraph();
		THROWEXCEPTION("Root element does not match \"ipfixConfig\"."
			       " This is not a valid configuration file!");
	}

	/* process each root element node and add a new node (with its config
	 * attached to the node) to the graph
	 */
	XMLNode::XMLSet<XMLElement*> rootElements = root->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::const_iterator it = rootElements.begin();
	     it != rootElements.end();
	     it++) {
		bool found = false;
		for (unsigned int i = 0; i < ARRAY_SIZE(configModules); i++) {
			if ((*it)->getName() == configModules[i]->getName()) {
				Cfg* cfg = configModules[i]->create(*it);

				// handle special modules
				SensorManagerCfg* smcfg = dynamic_cast<SensorManagerCfg*>(cfg);
				if (smcfg) {
					// SensorManager will not be connected to any modules, so its instance
					// needs to be started manually
					smcfg->setGraphIS(this);
					sensorManager = smcfg->getInstance();
				}

				graph->addNode(cfg);
				found = true;
			}
		}

		if (!found) {
			unlockGraph();
			msg(MSG_INFO, "Unknown cfg entry %s found", (*it)->getName().c_str());
		}
	}

	if (!oldGraph) { // this is the first config we have read
		Connector connector;
		graph->accept(&connector);
	} else {
		// first, connect the nodes on the new graph (but NOT the modules
		Connector connector(true, false);
		graph->accept(&connector);
		// now connect the modules reusing those from the old graph
		ReConnector reconnector(oldGraph);
		graph->accept(&reconnector);
	}

	// start the instances if not already running
	std::vector<CfgNode*> topoNodes = graph->topoSort();

	for (size_t i = 0; i < topoNodes.size(); i++) {
		Cfg* cfg = topoNodes[topoNodes.size() -1 -i]->getCfg();
		msg(MSG_INFO, "Starting module %s", cfg->getName().c_str());
		cfg->start(false);
	}

	if (old_document)
		delete old_document;

	// if there is an old graph, we did a reconfiguration. So now we have to delete the
	// old graph, but we can't delete the instances immediatly, because there is a small
	// chance that instances which got reused could still hold a reference to a instance we
	// want to delete right now.
	// => we use the deleter to delete the instances after a specific time has passed so we
	//    are safe that no-one holds a reference on the deleted modules anymore
	if (oldGraph)
		deleter.addGraph(oldGraph);

	unlockGraph();
}

void ConfigManager::shutdown()
{
	lockGraph();
	std::vector<CfgNode*> topoNodes = graph->topoSort();

	// shutdown modules
	for (size_t i = 0; i < topoNodes.size(); i++) {
		Cfg* cfg = topoNodes[i]->getCfg();
		msg(MSG_INFO, "shutting down module %s (id=%u)", cfg->getName().c_str(), cfg->getID());
		cfg->shutdown(true, true);
	}

	// trigger sensorManager to get the final statistics of this Vermont run
	if (sensorManager) {
		sensorManager->retrieveStatistics(true);
	}

	// disconnect the modules
	for (size_t i = 0; i < topoNodes.size(); i++) {
		CfgNode* n = topoNodes[i];
		Cfg* cfg = n->getCfg();

		// disconnect the module from its sources ..
		vector<CfgNode*> sources = graph->getSources(n);
		msg(MSG_INFO, "disconnecting module %s (id=%u)", cfg->getName().c_str(), cfg->getID());
		for (size_t k = 0; k < sources.size(); k++) {
			sources[k]->getCfg()->disconnectInstances();
		}
	}
	unlockGraph();
}


Graph* ConfigManager::getGraph()
{
	return graph;
}
