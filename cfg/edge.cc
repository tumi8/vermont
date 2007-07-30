#include "graph.h"

Node* Edge::nodeA()
{
	return graph->nodeA(NULL);
}

Node* Edge::NodeB()
{
	return graph->nodeB(this);
}
