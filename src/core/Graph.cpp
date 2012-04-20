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
#include "core/Edge.h"
#include "Connector.h"

#include <iostream>
#include <iterator>
#include <stdexcept>
#include <cstdlib>
#include <cstring>

Graph::Graph() : reserved(30)
{
	matrix = new Edge**[reserved];
	for (size_t i = 0; i < reserved; i++) {
		matrix[i] = new Edge*[reserved];
		for (size_t j = 0; j < reserved; j++)
			matrix[i][j] = NULL;
	}
}

Graph::~Graph()
{
	DPRINTF("~Graph()");
	std::vector<CfgNode*> ts = topoSort();

	for (size_t i = 0; i < reserved; i++) {
		for (size_t j = 0; j < nodes.size(); j++) {
			if (matrix[i][j] != NULL) // free all edges
				delete matrix[i][j];
			matrix[i][j] = NULL;
		}
		delete [] matrix[i];
		matrix[i] = NULL;
	}
	delete [] matrix;
	matrix = NULL;
	// we delete in topological order so that we never get into a race
	// where a module (which is deleted by the Cfg) is still running and wants to
	// send data to an module which got already freed. We rely on the d'tors to do
	// a join on the underlying thread.
	for (size_t i = 0; i < ts.size(); i++) {
		delete ts[i];
	}
	DPRINTF("~Graph() done");
}

CfgNode* Graph::addNode(Cfg* cfg)
{
	if (nodes.size() + 1  >= reserved)
		THROWEXCEPTION("Can't handle more than %d nodes", reserved);

	CfgNode *n = new CfgNode(this, nodes.size());
	n->setCfg(cfg);
	nodes.push_back(n);

	for (size_t i = 0; i < nodes.size(); i++) {
		matrix[i][nodes.size()-1] = NULL;
		matrix[nodes.size()-1][i] = NULL;
	}
	return n;
}

CfgNode* Graph::getNode(unsigned int id)
{
	if (id > nodes.size())
		return NULL;

	return nodes[id];
}

/* By removing a node have all the node ID's get remaped and you
 * would have to also remap all the Edges (which store the node ID's internally).
 * We don't do this here because it shouldn't be neccessary because the callers are only
 * interested in nodes and not on edges.
 */
void Graph::removeNode(Node* n)
{
	size_t nodes_org_size = nodes.size();
	size_t pos = n->getID();

	std::vector<CfgNode *>::iterator it = find(nodes.begin(), nodes.end(), n);
	if (it == nodes.end())
		throw std::runtime_error("node is not in the graph");

	Edge** oldLine = matrix[pos];

	// free all edges on that line
	for (size_t i = 0; i < nodes.size(); i++) {
		if (oldLine[i]) {
			delete oldLine[i];
			oldLine[i] = NULL;
		}
	}

	// remove the node from the array
	nodes.erase(it);

	// remove line
	for (size_t i = pos; i < nodes_org_size -1; i++) {
		nodes[i]->setID(i);
		matrix[i] = matrix[i+1];
	}


	// we reuse the allocated memory for the line so we reinsert it as the last line
	matrix[nodes_org_size - 1] = oldLine;

	// free the edges in the removed column
	for (size_t i = 0; i < nodes.size(); i++) {
		if (matrix[i][pos] != NULL) {
			delete matrix[i][pos];
			matrix[i][pos] = NULL;
		}
		memmove(&matrix[i][pos], &matrix[i][pos+1], (nodes_org_size - pos -1) * sizeof(Edge*));
		matrix[i][nodes_org_size-1] = NULL;
	}
}

Edge* Graph::addEdge(Node* n1, Node* n2)
{
	if (matrix[n1->getID()][n2->getID()] != NULL)
		 THROWEXCEPTION("Nodes are already connected");

	Edge* e = new Edge(this, n1->getID() << 16 | n2->getID());
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
	size_t A_pos = e->getID() >> 16;
	size_t B_pos = e->getID() & 0xFFFFU;

	if (matrix[A_pos][B_pos] == NULL)
		throw std::runtime_error("No edge between the nodes");

	matrix[A_pos][B_pos] = NULL;

	delete e;
}

Node* Graph::nodeA(Edge* e)
{
	size_t A_pos = e->getID() >> 16;
	size_t B_pos = e->getID() & 0xFFFFU;

	if (matrix[A_pos][B_pos] != e)
		throw std::runtime_error("edge != edge");

	return nodes[A_pos];
}

Node* Graph::nodeB(Edge* e)
{
	size_t A_pos = e->getID() >> 16;
	size_t B_pos = e->getID() & 0xFFFFU;

	if (matrix[A_pos][B_pos] != e)
		throw std::runtime_error("edge != edge");

	return nodes[B_pos];
}

/*
std::vector<Edge*> Graph::outgoingEdges(Node* n)
{
	std::vector<Edge*> outgoing;
	unsigned int a = n->getID();

	for (unsigned int b = 0; b < nodes.size(); b++) {
		Edge* e = matrix[a][b];
		if (e != NULL)
			outgoing.push_back(e);
	}
	return outgoing;
}
*/

/** return all the nodes we are connected to as a source of the connection */
std::vector<CfgNode*> Graph::getDestinations(Node* n) {
	std::vector<CfgNode*> result;
	size_t a = n->getID();

	for (size_t b = 0; b < nodes.size(); b++) {
		if (matrix[a][b] != NULL)
			result.push_back(nodes[b]);
	}
	return result;
}

/** return all the nodes we are connected to as a destination of the connection */
std::vector<CfgNode*> Graph::getSources(Node* n) {
	std::vector<CfgNode*> result;
	size_t b = n->getID();

	for (size_t a = 0; a < nodes.size(); a++) {
		if (matrix[a][b] != NULL)
			result.push_back(nodes[a]);
	}
	return result;
}

void Graph::depthSearch(Node* v)
{
	DPRINTFL(MSG_VDEBUG, "called (%u)", v->getID());
	preOrder[v->getID()] = cnt++;

	std::vector<CfgNode*> outNodes = getDestinations(v);
	for (std::vector<CfgNode*>::const_iterator it = outNodes.begin();
	     it != outNodes.end();
	     it++) {
		DPRINTFL(MSG_VDEBUG, "module %u -> module %u", v->getID(), (*it)->getID());
		Node* other = *it;
		if (preOrder[other->getID()] == -1)
			depthSearch(other);

	}

	postOrder[v->getID()] = topoCnt;
	msg(MSG_VDEBUG, "postI[%u] = %u", topoCnt, v->getID());
	postI[topoCnt++] = v->getID();
}

std::vector<CfgNode*> Graph::topoSort()
{
	topoCnt = cnt = 0;

	// initialise the values
	for (size_t i = 0; i < nodes.size(); i++) {
		visited.push_back(-1);
		preOrder.push_back(-1);
		postOrder.push_back(-1);
		postI.push_back(-1);
	}

	for (size_t i = 0; i < nodes.size(); i++) {
		DPRINTFL(MSG_VDEBUG, "NodeID=%u, Modulename=%s", nodes[i]->getID(), nodes[i]->getCfg()->getName().c_str());
		if (preOrder[i] == -1)
			depthSearch(nodes[i]);
	}

	size_t nz = nodes.size();
	std::vector<CfgNode*> result(nz);
	for (size_t i = 0; i < nz; i++) {
		result[nz-1-i] = nodes[postI[i]];
	}
	for (size_t i = 0; i < nz; i++) {
		msg(MSG_DEBUG, "topological sort #%u: %s[%u]", i, result[i]->getCfg()->getName().c_str(), result[i]->getCfg()->getID());
	}

	return result;
}

Graph* Graph::accept(Connector* c)
{
	return c->connect(this);
}

std::ostream& operator<< (std::ostream& o, const Graph& g)
{
	for (size_t from = 0; from < g.nodes.size(); from++) {
		o << " " << g.nodes[from]->getID();
	}
	o << std::endl;

	for (size_t from = 0; from < g.nodes.size(); from++) {
		o << g.nodes[from]->getID();
		for (size_t to = 0; to < g.nodes.size(); to++) {
			if (g.matrix[from][to] != NULL)
				o << " x";
			else
				o << "  ";
		}
		o << std::endl;
	}
	return o;
}
