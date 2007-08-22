#include "Graph.h"

inline Node* Edge::nodeA()
{
	return graph->nodeA(this);
}

inline Node* Edge::NodeB()
{
	return graph->nodeB(this);
}
