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
#include <ostream>

/**
	@author Peter Baumann <siprbaum@users.berlios.de>
*/

class Cfg;
class Connector;
class Edge;
class Node;
class CfgNode;

class Graph {
public:
	Graph();
	~Graph();

	friend std::ostream& operator<<(std::ostream& o, const Graph& g);

//	Node* addNode();
	CfgNode* addNode(Cfg* cfg);
	Edge* addEdge(Node* n1, Node* n2);

	CfgNode* getNode(unsigned int id);

	void removeNode(Node* n1);

	std::vector<Edge*> outgoingEdges(Node* n);

	/** return all the nodes we are connected to as a source of the connection */
	std::vector<CfgNode*> getDestinations(Node* n);

	/** return all the nodes we are connected to as a destination of the connection */
	std::vector<CfgNode*> getSources(Node* n);


	void removeEdge(Node* n1, Node* n2);
	void removeEdge(Edge* e);

	// returns the node from which the directed edge starts
	Node* nodeA(Edge* e);

	// returns the node from which the directed edge ends
	Node* nodeB(Edge* e);

	// returns a vector with all the nodes
	std::vector<CfgNode*> getNodes()
	{
		return nodes;
	}

	void depthSearch(Node* v);
	std::vector<CfgNode*> topoSort();

	Graph* accept(Connector* c);

private:
	std::vector<CfgNode*> nodes;

	unsigned int reserved;
	Edge*** matrix;

	// for topo sort
	std::vector<int> visited;
	std::vector<int> preOrder;
	std::vector<int> postOrder;
	std::vector<int> postI;
	unsigned int cnt, topoCnt;
};

#endif
