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
#include <vector>

#include "edge.h"
#include "node.h"

/**
	@author Peter Baumann <siprbaum@users.berlios.de>
*/

class Graph {
public:
	Graph();
	~Graph();

	friend std::ostream& operator<<(std::ostream& o, const Graph& g);

	Node* addNode();
	Edge* addEdge(Node* n1, Node* n2);

	void removeNode(Node* n1);

	void removeEdge(Node* n1, Node* n2);
	void removeEdge(Edge* e);

	// returns the node from which the directed edge starts
	Node* nodeA(Edge* e);

	// returns the node from which the directed edge ends
	Node* nodeB(Edge* e);

private:
	std::vector<Node*> nodes;
	std::vector<Edge*> edges;

	std::vector<unsigned int> start;

	unsigned int reserved;
	Edge*** matrix;
};

#endif
