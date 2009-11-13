#include "GraphInstanceSupplier.h"



void GraphInstanceSupplier::lockGraph()
{
	mutex.lock();
}

void GraphInstanceSupplier::unlockGraph()
{
	mutex.unlock();
}

bool GraphInstanceSupplier::tryLockGraph()
{
	return mutex.try_lock();
}
