#include <cfg/ReConnector.h>
#include <cfg/CfgNode.h>

#include <msg.h>

Graph* ReConnector::connect(Graph* g)
{
	newGraph = g;

	vector<CfgNode*> commonNodes;

	vector<CfgNode*> topoOld = oldGraph->topoSort();
	vector<CfgNode*> topoNew = newGraph->topoSort();


	// compare the nodes in the old and new graph and search for
	// (nearly) identicall modules which could be reused
	for (size_t i = 0; i < topoOld.size(); i++) {
		Cfg* oldCfg = topoOld[i]->getCfg();

		for (size_t j = 0; j < topoNew.size(); j++) {
			Cfg* newCfg = topoNew[j]->getCfg();
			if (oldCfg->getID() == newCfg->getID()) { // possible match
				msg(MSG_FATAL, "\nFOUND A MATCH BETWEEN %d -> %d\n\n\n", oldCfg->getID(), newCfg->getID());
				commonNodes.push_back(topoOld[i]);

				// check if we could use the same module instance in the new config
				if (newCfg->deriveFrom(oldCfg)) {
					fprintf(stderr, "---starting disconnect for %d\n", oldCfg->getID());

					// disconnect the module form its sources ..
					vector<CfgNode*> sources = oldGraph->getSources(topoOld[i]);
					for (size_t k = 0; k < sources.size(); k++) {
						fprintf(stderr, "---call ing disconnect on %d\n", sources[k]->getCfg()->getID());
						sources[k]->getCfg()->disconnectInstances();
					}

					newCfg->transferInstance(oldCfg);
					
					// ... and from its destination
					newCfg->disconnectInstances();
					fprintf(stderr, "---ending disconnect for %d\n", oldCfg->getID());
				}
			}
		}
	}


	msg(MSG_FATAL, "\nEND match");

	// Now that we transfered all module instances which could be reused
	// into the new graph, we have to build up the new connections
	Connector con(false, true);
	newGraph->accept(&con);

	return newGraph;
}
