//
// C++ Implementation: graph
//
// Description:
//
//
// Author: Peter Baumann <siprbaum@users.berlios.de>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "Graph.h"
#include "CfgNode.h"

#include <iostream>
#include <iterator>
#include <stdexcept>
#include <cstdlib>
#include <cstring>

Graph::Graph() : reserved(10)
{
	matrix = new Edge**[reserved];
	for (unsigned int i = 0; i < reserved; i++) {
		matrix[i] = new Edge*[reserved];
		for (unsigned int j = 0; j < reserved; j++)
			matrix[i][j] = NULL;
	}
}

Graph::~Graph()
{
}

/*
Node* Graph::addNode()
{
	if (nodes.size() + 1  >= reserved)
		throw std::runtime_error("can't handle that many nodes");

	Node *n = new Node(this, nodes.size());
	nodes.push_back(n);

	return n;
}
*/

CfgNode* Graph::addNode(Cfg* cfg)
{
	if (nodes.size() + 1  >= reserved)
		throw std::runtime_error("can't handle that many nodes");

	CfgNode *n = new CfgNode(this, nodes.size());
	n->addCfg(cfg);
	nodes.push_back(n);

	return n;
}


void Graph::removeNode(Node* n)
{
	unsigned int nodes_org_size = nodes.size();
	unsigned int pos = n->getID();

	std::vector<CfgNode *>::iterator it = find(nodes.begin(), nodes.end(), n);
	if (it == nodes.end())
		throw std::runtime_error("node is not in the graph");


	delete [] matrix[pos];

	// remove the node from the array
	nodes.erase(it);

	// remove line
	for (unsigned int i = pos; i < nodes_org_size -1; i++) {
		nodes[i]->setID(i);
		matrix[i] = matrix[i+1];
	}

	//remove column
	for (unsigned int i = 0; i < nodes.size(); i++) {
		for (unsigned int j = pos + 1; j < nodes_org_size; j++) {
			matrix[i][j-1] = matrix[i][j];
		}
	}
}

Edge* Graph::addEdge(Node* n1, Node* n2)
{
	if (matrix[n1->getID()][n2->getID()] != NULL)
		 throw std::runtime_error("Nodes are already connected");

	Edge* e = new Edge(this, n1->getID() << 16 | n2->getID());
	edges.push_back(e);
	matrix[n1->getID()][n2->getID()] = e;
	return e;
}

void Graph::removeEdge(Node* n1, Node* n2)
{
	if (matrix[n1->getID()][n2->getID()] < 0)
		throw std::runtime_error("No edge between the nodes");

	Edge *e = matrix[n1->getID()][n2->getID()];
	matrix[n1->getID()][n2->getID()] = NULL;
	delete e;
}

void Graph::removeEdge(Edge* e)
{
	unsigned int A_pos = e->getID() >> 16;
	unsigned int B_pos = e->getID() & 0xFFFFU;

	if (matrix[A_pos][B_pos] == NULL)
		throw std::runtime_error("No edge between the nodes");

	matrix[A_pos][B_pos] = NULL;

	delete e;
}

Node* Graph::nodeA(Edge* e)
{
	unsigned int A_pos = e->getID() >> 16;
	unsigned int B_pos = e->getID() & 0xFFFFU;

	if (matrix[A_pos][B_pos] != e)
		throw std::runtime_error("edge != edge");

	return nodes[A_pos];
}

Node* Graph::nodeB(Edge* e)
{
	unsigned int A_pos = e->getID() >> 16;
	unsigned int B_pos = e->getID() & 0xFFFFU;

	if (matrix[A_pos][B_pos] != e)
		throw std::runtime_error("edge != edge");

	return nodes[B_pos];
}

Graph* Graph::accept(Connector* c)
{
	return c->connect(this, nodes);
}


std::ostream& operator<< (std::ostream& o, const Graph& g)
{
	o << " ";
	for (unsigned int from = 0; from < g.nodes.size(); from++) {
		o << " " << g.nodes[from]->getID();
	}
	o << std::endl;

	for (unsigned int from = 0; from < g.nodes.size(); from++) {
		o << g.nodes[from]->getID();
		for (unsigned int to = 0; to < g.nodes.size(); to++) {
			if (g.matrix[from][to] != NULL)
				o << " x";
			else
				o << "  ";
		}
		o << std::endl;
	}
	return o;
}
