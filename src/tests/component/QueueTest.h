#if !defined(QUEUETEST_H)
#define QUEUETEST_H

#include "common/ConcurrentQueue.h"
//delete after use
#include "common/BaseQueue.h"
#include "common/STLQueue.h"
#include "common/LockfreeSingleQueue.h"
#include "common/LockfreeMultiQueue.h"

#include "common/Thread.h"
#include "common/msg.h"


class QueueTest{
	public:
		QueueTest(uint32_t queueType, uint32_t numProducer, uint32_t numConsumers,
				uint32_t queueSize, uint32_t timeoutLength);
		~QueueTest();
		struct timespec runTest(uint32_t numOps);
		static void* pushFunc(void*);
		static void* popFunc(void*);

		volatile uint32_t numProducer;
		volatile uint32_t numConsumer;
		volatile uint32_t queueSize;
		volatile uint32_t numOps;
		volatile uint32_t timeoutLength;

		Thread* pusher[20];
		Thread* popper[20];

		BaseConcurrentQueue<uint32_t>* queue;
};


#endif
