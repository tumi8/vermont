#ifndef RECONNECTOR_H_
#define RECONNECTOR_H_
/**
	@author Peter Baumann <siprbaum@users.berlios.de>
*/

#include "Connector.h"

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

private:
	Graph* oldGraph;
	Graph* newGraph;
};

#endif /*RECONNECTOR_H_*/
