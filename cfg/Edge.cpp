#include "Graph.h"

inline Node* Edge::nodeA()
{
	return graph->nodeA(NULL);
}

inline Node* Edge::NodeB()
{
	return graph->nodeB(this);
}
