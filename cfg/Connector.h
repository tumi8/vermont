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

	Connector(bool connectNodes = true, bool connectModules = true)
		: connectNodes(connectNodes), connectModules(connectModules)
	{
	}

	virtual ~Connector() { }

	virtual Graph* connect(Graph*g);

protected:
	std::map<unsigned int, CfgNode*> getId2Node(const std::vector<CfgNode*>& nodes);

	bool connectNodes;
	bool connectModules;
};

class ReConnector
	: public Connector
{
public:
	ReConnector(Graph* g, bool connectModules = true)
		: Connector(true, connectModules), oldGraph(g)
	{

	}

	virtual ~ReConnector() { }

	virtual Graph* connect(Graph* g);

	std::vector<CfgNode*> getCommonNodes();

private:
	Graph* oldGraph;
	Graph* newGraph;
};

#endif /*CONNECTOR_H_*/
