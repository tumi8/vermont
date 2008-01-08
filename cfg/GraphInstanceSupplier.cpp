#include "GraphInstanceSupplier.h"



void GraphInstanceSupplier::lockGraph()
{
	mutex.lock();
}

void GraphInstanceSupplier::unlockGraph()
{
	mutex.unlock();
}
