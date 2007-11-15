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

	/** c'tor
	 * @param connectNodes set it to true if you want that the nodes  should be connected
	 * @param connectModules set to true if you want the modules/instances to be connected
	 */
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

#endif /*CONNECTOR_H_*/
