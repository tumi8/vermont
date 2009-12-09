#include <core/ReConnector.h>
#include <core/CfgNode.h>

#include "common/msg.h"

Graph* ReConnector::connect(Graph* g)
{
	newGraph = g;

	vector<CfgNode*> topoOld = oldGraph->topoSort();
	vector<CfgNode*> topoNew = newGraph->topoSort();

	/* disconnect all modules */
	for (size_t i = 0; i < topoOld.size(); i++) {
		topoOld[i]->getCfg()->getInstance()->preReconfiguration();
		topoOld[i]->getCfg()->disconnectInstances();
	}

	/* call onReconfiguration1 on all modules */
	for (size_t i = 0; i < topoOld.size(); i++) {
		topoOld[i]->getCfg()->onReconfiguration1();
	}

	/* call preConfiguration2 on all modules */
	for (size_t i = 0; i < topoOld.size(); i++) {
		topoOld[i]->getCfg()->onReconfiguration2();
	}


	// compare the nodes in the old and new graph and search for
	// (nearly) identical modules which could be reused
	for (size_t i = 0; i < topoOld.size(); i++) {
		Cfg* oldCfg = topoOld[i]->getCfg();
		for (size_t j = 0; j < topoNew.size(); j++) {
			Cfg* newCfg = topoNew[j]->getCfg();
			if (oldCfg->getID() == newCfg->getID()) { // possible match
				msg(MSG_INFO, "found a match between %s(id=%d) -> %s(id=%d)",
						oldCfg->getName().c_str(), oldCfg->getID(),
						newCfg->getName().c_str(), newCfg->getID());

				// check if we could use the same module instance in the new config
				if (newCfg->deriveFrom(oldCfg)) {
					msg(MSG_INFO, "reusing %s(id=%d)",
							oldCfg->getName().c_str(), oldCfg->getID());
					newCfg->transferInstance(oldCfg);
				} else {
					msg(MSG_INFO, "can't reuse %s(id=%d)",
							oldCfg->getName().c_str(), oldCfg->getID());
				}
			}
		}
	}

	/* Now that we transfered all module instances which could be reused
	 * into the new graph, we have to build up the new connections
	 *
	 * The Connector will take care to call preConnect for us!!!
	 */
	Connector con(false, true);
	newGraph->accept(&con);

	return newGraph;
}
