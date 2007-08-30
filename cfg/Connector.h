#ifndef CONNECTOR_H_
#define CONNECTOR_H_
/**
	@author Peter Baumann <siprbaum@users.berlios.de>
*/

#include "cfg/Graph.h"

class Connector
{
public:
	friend class Graph;

	Connector() { };
	virtual ~Connector() { };

	virtual Graph* connect(Graph*g);

protected:
	std::map<unsigned int, CfgNode*> getId2Node(const std::vector<CfgNode*>& nodes);
};

#endif /*CONNECTOR_H_*/
