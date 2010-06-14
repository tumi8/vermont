#if !defined(QUEUETEST_H)
#define QUEUETEST_H

#include "common/ConcurrentQueue.h"
#include "common/Thread.h"
#include "common/msg.h"


class QueueTest{
	public:
		QueueTest(int queueType, int numProducer, int queueSize);
		~QueueTest();
		struct timespec runTest(int numOps);
		static void* pushFunc(void*);
		static void* popFunc(void*);

		int queueType;
		int numProducer;
		int queueSize;
		int numOps;

		Thread* pusher[20];
		Thread* popper;

		ConcurrentQueue<int>* queue;
};


#endif
