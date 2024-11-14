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

#include "ConfigManager.hpp"
#include "core/Connector.h"
#include "core/CfgNode.h"
#include "common/defs.h"

#include "core/Source.h"
#include "QueueCfg.h"
#include "AnonymizerCfg.h"
#include "modules/packet/ObserverCfg.h"
#include "modules/packet/PSAMPExporterCfg.h"
#include "modules/packet/PCAPExporterFileCfg.h"
#include "modules/packet/PCAPExporterPipeCfg.h"
#include "modules/packet/filter/PacketFilterCfg.h"
#include "modules/ipfix/FpaPcapExporterCfg.h"
#include "modules/ipfix/FpaPacketGeneratorCfg.h"
#include "modules/ipfix/IpfixCollectorCfg.h"
#include "modules/ipfix/IpfixExporterCfg.h"
#include "modules/ipfix/IpfixPrinterCfg.h"
#include "modules/ipfix/IpfixFileWriterCfg.hpp"
#include "modules/ipfix/IpfixNetflowExporterCfg.h"
#include "modules/ipfix/IpfixReceiverFileCfg.h"
#include "modules/ipfix/IpfixPayloadWriterCfg.h"
#include "modules/ipfix/IpfixSamplerCfg.h"
#include "modules/ipfix/IpfixCsExporterCfg.hpp"
#include "modules/ipfix/NetflowV9ConverterCfg.hpp"
#include "modules/ipfix/aggregator/IpfixAggregatorCfg.h"
#include "modules/ipfix/aggregator/PacketAggregatorCfg.h"
#include "modules/ipfix/database/IpfixDbReaderCfg.h"
#include "modules/ipfix/database/IpfixDbWriterCfg.h"
#include "modules/ipfix/database/IpfixDbWriterMongoCfg.h"
#include "modules/ipfix/database/IpfixFlowInspectorExporterCfg.h"
#include "modules/SensorManagerCfg.h"
#include "modules/analysis/TRWPortscanDetectorCfg.h"
#include "modules/analysis/RBSWormDetectorCfg.h"
#include "modules/analysis/FrontPayloadSigMatcherCfg.h"
#include "modules/analysis/AutoFocusCfg.h"
#include "modules/analysis/FlowLenAnalyzerCfg.h"
#include "modules/idmef/IDMEFExporterCfg.h"
#include "modules/idmef//PacketIDMEFReporterCfg.h"
#include "modules/analysis/P2PDetectorCfg.h"
#include "modules/analysis/HostStatisticsCfg.h"

#include <cassert>

// we create a static array of all root config entrys so that we don't
// need to hardcode the config entry name in here. Instead, we just ask the
// module instances if they handle the specific entry.
Cfg* ConfigManager::configModules[] = {
	new ObserverCfg(NULL),
	new PacketFilterCfg(NULL),
	new PacketQueueCfg(NULL),
	new PCAPExporterFileCfg(NULL),
	new PCAPExporterPipeCfg(NULL),
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
	new IpfixNetflowExporterCfg(NULL),
	new IpfixPayloadWriterCfg(NULL),
	new IpfixFileWriterCfg(NULL),
	new AnonymizerCfg(NULL),
	new FrontPayloadSigMatcherCfg(NULL),
	new P2PDetectorCfg(NULL),
	new HostStatisticsCfg(NULL),
	new IpfixCsExporterCfg(NULL),
#if defined(DB_SUPPORT_ENABLED) || defined(PG_SUPPORT_ENABLED) || defined(ORACLE_SUPPORT_ENABLED)
	new IpfixDbWriterCfg(NULL),
	new IpfixDbReaderCfg(NULL),
#endif
#ifdef MONGO_SUPPORT_ENABLED
	new IpfixDbWriterMongoCfg(NULL),
#endif
#ifdef REDIS_SUPPORT_ENABLED
	new IpfixFlowInspectorExporterCfg(NULL),
#endif
	new FlowLenAnalyzerCfg(NULL),
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
	if (document) {
		delete document;
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

	XMLAttribute* logging_attribute = root->getAttribute("logging");
	if (logging_attribute) {
		int log_bitask = parse_log_level(logging_attribute->getValue().c_str());
		if (log_bitask == -1) {
			msg(LOG_CRIT, "ignoring unknown log level '%s'",
			    logging_attribute->getValue().c_str());
		} else {
			msg(LOG_NOTICE, "setting log level '%s'",
			    logging_attribute->getValue().c_str());
			msg_setlevel(log_bitask);
		}
	}

	/* process each root element node and add a new node (with its config
	 * attached to the node) to the graph
	 */
	XMLNode::XMLSet<XMLElement*> rootElements = root->getElementChildren();
	for (const auto& element : rootElements) {
		bool found = false;
		for (auto& module : configModules) {
			if (element->getName() == module->getName()) {
				Cfg* cfg = module->create(element);

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
			msg(LOG_ERR, "Unknown cfg entry %s found", element->getName().c_str());
		}
	}

	if (!oldGraph) { // this is the first config we have read
		Connector connector;
		graph->accept(&connector);
	} else {
		// first, connect the nodes on the new graph (but NOT the modules)
		Connector connector(true, false);
		graph->accept(&connector);
		// now connect the modules reusing those from the old graph
		graph = reconnect(graph, oldGraph);
	}

	// start the instances if not already running
	std::vector<CfgNode*> topoNodes = graph->topoSort();

	for (size_t i = 0; i < topoNodes.size(); i++) {
		Cfg* cfg = topoNodes[topoNodes.size() -1 -i]->getCfg();
		msg(LOG_NOTICE, "Starting module %s", cfg->getName().c_str());
		cfg->start(false);
	}

	if (old_document)
		delete old_document;

	unlockGraph();

}

void ConfigManager::shutdown()
{
	lockGraph();
	std::vector<CfgNode*> topoNodes = graph->topoSort();

	// shutdown modules
	for (size_t i = 0; i < topoNodes.size(); i++) {
		Cfg* cfg = topoNodes[i]->getCfg();
		msg(LOG_NOTICE, "shutting down module %s (id=%u)", cfg->getName().c_str(), cfg->getID());
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
		msg(LOG_NOTICE, "disconnecting module %s (id=%u)", cfg->getName().c_str(), cfg->getID());
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

void ConfigManager::onTimeout2()
{
	//msg(LOG_DEBUG, "Called deleter");

	for (std::list<deleter_list_item>::iterator it = deleter_list.begin(); it != deleter_list.end(); it++) {
		if (time(NULL) > it->delete_after) {
			msg(LOG_INFO, "Removing node: %s", (it->c)->getName().c_str());
			(it->c)->shutdown(true, true);
			it->c->disconnectInstances();
			delete ((it->c));
			it = deleter_list.erase(it);
			it--;
		} else {
			msg(LOG_INFO, "Timeout for node %s not yet reached.", (it->c)->getName().c_str());
		}
	}
}

Graph* ConfigManager::reconnect(Graph* g, Graph *old)
{
    Graph *newGraph;
    Graph *oldGraph;
	newGraph = g;
    oldGraph = old;

	vector<CfgNode*> topoOld = oldGraph->topoSort();
	vector<CfgNode*> topoNew = newGraph->topoSort();

	/* disconnect all modules */
	for (size_t i = 0; i < topoOld.size(); i++) {
		topoOld[i]->getCfg()->getInstance()->preReconfiguration();
		topoOld[i]->getCfg()->disconnectInstances();
        msg(LOG_NOTICE, "Disconnecting instance: %s", topoOld[i]->getCfg()->getName().c_str());
	}

	/* call onReconfiguration1 on all modules */
	for (size_t i = 0; i < topoOld.size(); i++) {
		topoOld[i]->getCfg()->onReconfiguration1();
	}

	/* call preConfiguration2 on all modules */
	for (size_t i = 0; i < topoOld.size(); i++) {
		topoOld[i]->getCfg()->onReconfiguration2();
	}


	// compare the nodes in the old and new graph and search for
	// (nearly) identical modules which could be reused
	for (size_t i = 0; i < topoOld.size(); i++) {
		Cfg* oldCfg = topoOld[i]->getCfg();
		for (size_t j = 0; j < topoNew.size(); j++) {
			Cfg* newCfg = topoNew[j]->getCfg();
			if (oldCfg->getID() == newCfg->getID()) { // possible match
				msg(LOG_NOTICE, "found a match between %s(id=%u) -> %s(id=%u)",
						oldCfg->getName().c_str(), oldCfg->getID(),
						newCfg->getName().c_str(), newCfg->getID());

				// check if we could use the same module instance in the new config
				if (newCfg->deriveFrom(oldCfg)) {
					msg(LOG_NOTICE, "reusing %s(id=%u)",
							oldCfg->getName().c_str(), oldCfg->getID());
					newCfg->transferInstance(oldCfg);
				} else {
					deleter_list_item delme;
					delme.c = oldCfg;
					delme.delete_after = time(NULL) + DELETER_DELAY; // current time + 20 seconds
					deleter_list.push_back(delme);
					msg(LOG_NOTICE, "can't reuse %s(id=%u)",
						oldCfg->getName().c_str(), oldCfg->getID());
				}
			}
		}
	}


	/* Now that we transfered all module instances which could be reused
	 * into the new graph, we have to build up the new connections
	 *
	 * The Connector will take care to call preConnect for us!!!
	 */
	Connector con(false, true);
	newGraph->accept(&con);

	return newGraph;
}
