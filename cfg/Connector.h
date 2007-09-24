#ifndef CONNECTOR_H_
#define CONNECTOR_H_
/**
	@author Peter Baumann <siprbaum@users.berlios.de>
*/

//#include "cfg/Graph.h"

#include <map>

#include "Graph.h"

/**
 * This class is used to build up the configuration graph.
 * Normally it gets a Graph* which isn't connected yet (it has
 * only nodes, no edges).
 * The nodes are traversed topologically and then connected.
 */
class Connector
{
public:
	friend class Graph;

	Connector(bool connectNodes = true, bool connectModules = true)
		: connectNodes(connectNodes), connectModules(connectModules)
	{
	}

	virtual ~Connector() { }

	/**
	 * Connects the nodes in the Graph
	 * @param g Graph which should be connected
	 */
	virtual Graph* connect(Graph* g);

protected:
	std::map<unsigned int, CfgNode*> getId2Node(const std::vector<CfgNode*>& nodes);

	bool connectNodes;
	bool connectModules;
};

/**
 * This class is initialized with an already connected graph and calling
 * its connect method with an unconnected graph tries to reuse module
 * instances (e.g. Observer, Filter etc) from the old graph if the
 * configuration of the old and new module are similar.
 *
 * This is an optimization prevent destroying and old module and recreating it
 * with the same configuration in the new graph.
 */
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
