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

Graph* Connector::connect(Graph* g)
{
	const std::vector<CfgNode*>& nodes = g->getNodes();

	std::map<unsigned int, CfgNode*> id2node = getId2Node(nodes);

	for (unsigned int i = 0; i < nodes.size(); i++) {
		CfgNode* fromNode = nodes[i];
		Cfg* cfg = fromNode->getCfg();

		std::vector<unsigned int> nexts = cfg->getNext();

		if (nexts.size()==0) {
			cfg->setupWithoutSuccessors();
		} else {
			for (unsigned int j = 0; j < nexts.size(); j++) {
				CfgNode* toNode = id2node[nexts[j]];

				if (toNode == NULL)
					THROWEXCEPTION("next statement is illegal; there is no node with id=%d", nexts[j]);

				if (connectNodes) // insert the connection in the graph
					g->addEdge(fromNode, toNode);

				msg(MSG_INFO,  "Connecting module %s[Id = %d] -> %s[Id = %d]",
						cfg->getName().c_str(), cfg->getID(),
						id2node[nexts[j]]->getCfg()->getName().c_str(),
						id2node[nexts[j]]->getCfg()->getID());

				if (connectModules) {// connect the modules
					DPRINTF("connecting instances");
					cfg->connectInstances(toNode->getCfg());
				}
			}
		}
	}

	return g;
}
