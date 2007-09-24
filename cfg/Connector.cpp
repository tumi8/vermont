#include "Connector.h"
#include "CfgNode.h"

#include <algorithm>
#include <iterator>

using std::max;
using std::vector;


std::map<unsigned int, CfgNode*> Connector::getId2Node(const std::vector<CfgNode*>& nodes)
{
	std::map<unsigned int, CfgNode*> id2node;

	// establish a mapping from id -> node for easier access
	for (unsigned int i = 0; i < nodes.size(); i++) {
		unsigned int id = nodes[i]->getCfg()->getID();
		id2node[id] = nodes[i];
	}

	return id2node;
}

Graph* Connector::connect(Graph*g)
{
	const std::vector<CfgNode*>& nodes = g->getNodes();

	std::map<unsigned int, CfgNode*> id2node = getId2Node(nodes);

	for (unsigned int i = 0; i < nodes.size(); i++) {
		CfgNode* fromNode = nodes[i];
		Cfg* cfg = fromNode->getCfg();

		std::vector<unsigned int> nexts = cfg->getNext();

		 // FIXME: Splicer case
		if (nexts.size() > 1) {
			THROWEXCEPTION("You need a splicer; not yet implemented\n");
		}

		for (unsigned int j = 0; j < nexts.size(); j++) {
			msg(MSG_FATAL, "to ID %d", nexts[j]);
			CfgNode* toNode = id2node[nexts[j]];

			if (toNode == NULL)
				THROWEXCEPTION("next statement is illegal; there is no node with id=%d", nexts[j]);

			if (connectNodes) // insert the connection in the graph
				g->addEdge(fromNode, toNode);

			msg(MSG_FATAL, "Connectiong %d -> %d", cfg->getID(), id2node[nexts[j]]->getCfg()->getID());

			if (connectModules) {// connect the modules
				msg(MSG_FATAL, "\n\nConnect Instances\n");
				cfg->connectInstances(toNode->getCfg());
			}
		}
	}

	return g;
}

Graph* ReConnector::connect(Graph* g)
{
	newGraph = g;

	vector<CfgNode*> commonNodes;

	vector<CfgNode*> topoOld = oldGraph->topoSort();
	vector<CfgNode*> topoNew = newGraph->topoSort();


	// compare the nodes in the old and new graph and search for
	// (nearly) identicall modules which could be reused
	for (size_t i = 0; i < topoOld.size(); i++) {
		Cfg* oldCfg = topoOld[i]->getCfg();

		for (size_t j = 0; j < topoNew.size(); j++) {
			Cfg* newCfg = topoNew[j]->getCfg();
			if (oldCfg->getID() == newCfg->getID()) { // possible match
				msg(MSG_FATAL, "\nFOUND A MATCH BETWEEN %d -> %d\n\n\n", oldCfg->getID(), newCfg->getID());
				commonNodes.push_back(topoOld[i]);

				// check if we could use the same module instance in the new config
				if (newCfg->deriveFrom(oldCfg)) {
					// FIXME: splitter handling

					fprintf(stderr, "---starting disconnect for %d\n", oldCfg->getID());

					// disconnect the module form its sources ..
					vector<CfgNode*> sources = oldGraph->getSources(topoOld[i]);
					for (size_t k = 0; k < sources.size(); k++) {
						fprintf(stderr, "---call ing disconnect on %d\n", sources[k]->getCfg()->getID());
						sources[k]->getCfg()->getInstance()->disconnect();
					}

					// ... and from its destination
					newCfg->getInstance()->disconnect();
					fprintf(stderr, "---ending disconnect for %d\n", oldCfg->getID());
				}
			}
		}
	}


	msg(MSG_FATAL, "\nEND match");

	// Now that we transfered all module instances which could be reused
	// into the new graph, we have to build up the new connections
	Connector con(false, true);
	newGraph->accept(&con);

	return newGraph;
}
