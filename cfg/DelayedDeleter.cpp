#include "DelayedDeleter.h"

#include <algorithm>
#include <cassert>
#include <iterator>

#include "cfg/CfgNode.h"

// try out some STL template magic
static inline void delme(CfgNode* node)
{
	delete node;
}

DelayedDeleter::DelayedDeleter()
{
	timer.start();
}

DelayedDeleter::~DelayedDeleter()
{
	timer.shutdown();

#if defined(DEBUG)
	mutex.lock();
	// try out some STL template magic
	// this will run foreach(listelement) { delete listelement }
	std::for_each(list.begin(), list.end(), ptr_fun(delme));
	mutex.unlock();
#endif
}

void DelayedDeleter::addCfgNode(CfgNode* node)
{
	struct timespec deadline;

#if defined(DEBUG)
	mutex.lock();
#endif

	addToCurTime(&deadline, wait_in_ms);


	timer.addTimeout(this, deadline, (void*)node);

#if defined(DEBUG)
	list.push_back(node);
	mutex.unlock();
#endif
}

void DelayedDeleter::addGraph(Graph* graph)
{
	std::vector<CfgNode*> nodes = graph->getNodes();
	for (std::vector<CfgNode*>::iterator it = nodes.begin();
	     it != nodes.end(); it++) {
		graph->removeNode(*it);

		addCfgNode(*it);
	}

	delete graph;
}

void DelayedDeleter::onTimeout(void* dataPtr)
{
	CfgNode* node = (CfgNode*)dataPtr;

#if defined(DEBUG)
	mutex.lock();
	std::list<CfgNode*>::iterator it = find(list.begin(), list.end(), node);

	// the element must be in the list!!!
	assert(it != list.end());

	list.erase(it);
	mutex.unlock();
#endif

	delete node;


}
