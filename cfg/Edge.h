//
// C++ Interface: edge
//
// Description: 
//
//
// Author: Peter Baumann <siprbaum@users.berlios.de>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef EDGE_H
#define EDGE_H


/**
	@author Peter Baumann <siprbaum@users.berlios.de>
*/

class Node;
class Graph;


class Edge {
private:
	Graph* graph;
	unsigned int id;

public:

	Edge(Graph* g, unsigned int id) : graph(g), id(id) { };
	~Edge() { }

	unsigned int getID() { return id; }

	inline Node* nodeA()
	{
		return graph->nodeA(this);
	}

	inline Node* NodeB()
	{
		return graph->nodeB(this);
	}
};

#endif
