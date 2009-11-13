#include "Node.h"
#include "Graph.h"

#include <iostream>

Node::Node(Graph *g, unsigned int id) : graph(g), id(id) { }

Node::~Node() { }

unsigned int Node::getID() 
{ 
	return id;
}

void Node::setID(unsigned int id)
{ 
	this->id = id;
}
