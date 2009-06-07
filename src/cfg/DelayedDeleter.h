#ifndef DELAYEDDELETER_H_
#define DELAYEDDELETER_H_

#include "Graph.h"
#include "reconf/ConnectionQueue.h"
#include "reconf/Emitable.h"
#include "reconf/Notifiable.h"

#include <list>

/** This class deletes a CfgNode after a timeout has passed
 *  It is neccessary because otherwise we could derefence a pointer on an already
 *  deleted object in one of the module instances
 */
class DelayedDeleter
	: public Notifiable
{
public:
	DelayedDeleter();
	~DelayedDeleter();

	/** add the node for deletion
	 *  @param node the node which should get deleted
	 */
	void addCfgNode(CfgNode* node);

	/** helper function to mark all nodes of a graph for deletion
	 *  @param graph the graph on which all nodes should be deleted
	 */
	void addGraph(Graph* graph);

	// see Notifiable
	virtual void onTimeout(void* dataPtr);

private:
	std::list<CfgNode*> list;
	ConnectionQueue<NullEmitable*> timer;

	static const long wait_in_ms = 10000; // wait 10 * 1000ms = 10s

	Mutex mutex;
};

#endif /*DELAYEDDELETER_H_*/
