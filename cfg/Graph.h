//
// C++ Interface: graph
//
// Description:
//
//
// Author: Peter Baumann <siprbaum@users.berlios.de>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef GRAPH_H
#define GRAPH_H

#include <cstdio>
#include <map>
#include <vector>

#include "Edge.h"
#include "CfgNode.h"

#include "Cfg.h"

/**
	@author Peter Baumann <siprbaum@users.berlios.de>
*/

class Connector;

class Graph {
public:
	Graph();
	~Graph();

	friend std::ostream& operator<<(std::ostream& o, const Graph& g);

//	Node* addNode();
	CfgNode* addNode(Cfg* cfg);
	Edge* addEdge(Node* n1, Node* n2);

	void removeNode(Node* n1);

	void removeEdge(Node* n1, Node* n2);
	void removeEdge(Edge* e);

	// returns the node from which the directed edge starts
	Node* nodeA(Edge* e);

	// returns the node from which the directed edge ends
	Node* nodeB(Edge* e);

	Graph* accept(Connector* c);
	
private:
	std::vector<CfgNode*> nodes;
	std::vector<Edge*> edges;

	std::vector<unsigned int> start;

	unsigned int reserved;
	Edge*** matrix;
};

class Connector
{
public:
	Connector() { };
	virtual ~Connector() { };

	virtual Graph* connect(Graph*g, std::vector<CfgNode*>& nodes)
	{
		std::map<unsigned int, CfgNode*> id2node;

		// establish a mapping from id -> node for easier access
		for (unsigned int i = 0; i < nodes.size(); i++) {
			unsigned int id = nodes[i]->getCfg()->getID();
			id2node[id] = nodes[i];
		}

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
};


#endif
