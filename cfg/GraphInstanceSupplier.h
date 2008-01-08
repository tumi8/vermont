#ifndef GRAPHINSTANCESUPPLIER_H_
#define GRAPHINSTANCESUPPLIER_H_

#include "Graph.h"

#include "common/Mutex.h"


/**
 * manages lock for module graph, as it is accessed by the main thread
 * and the SensorManager simultaneously
 * this interface is implemented by ConfigManager
 */
class GraphInstanceSupplier
{
public:
	virtual ~GraphInstanceSupplier() {}
	
	virtual void lockGraph();
	virtual bool tryLockGraph();
	virtual void unlockGraph();
	virtual Graph* getGraph() = 0;

private:
	Mutex mutex;
};

#endif /*GRAPHINSTANCESUPPLIER_H_*/
