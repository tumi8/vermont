#include "Connector.h"

#include <algorithm>
#include <iterator>


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

			// insert the connection in the graph
			g->addEdge(fromNode, toNode);

			msg(MSG_FATAL, "Connectiong %d -> %d", cfg->getID(), id2node[nexts[j]]->getCfg()->getID());

			// connect the modules
			cfg->connectInstances(toNode->getCfg());
		}
	}

	return g;
}

