#include "QueueTest.h"
#include "common/Thread.h"

#include <stdio.h>

QueueTest::QueueTest(uint32_t queueType, uint32_t numProducer, uint32_t queueSize):
	numProducer(numProducer),
	queueSize(queueSize),
	queue(NULL)
{
	if(numProducer > 20)
		THROWEXCEPTION("failed to create new thread");

	//creation of the threads
	for(uint32_t i=0; i<numProducer; i++){
		pusher[i] = new Thread(QueueTest::pushFunc);
	}
	popper = new Thread(QueueTest::popFunc);

	//creation of the queue

	//with ConcurrentQueue
	//Old Queue with lock
	//if(queueType == 1)
	//	queue = new ConcurrentQueue<uint32_t>(ConcurrentQueue<uint32_t>::STL, queueSize);
	//lockfree Queue for single producers
	//else if(queueType == 2 && numProducer == 1)
	//	queue = new ConcurrentQueue<uint32_t>(ConcurrentQueue<uint32_t>::LOCKFREE_SINGLE, queueSize);
	//lockfree Queue for multi producers
	//else
	//	queue = new ConcurrentQueue<uint32_t>(ConcurrentQueue<uint32_t>::LOCKFREE_MULTI, queueSize);

	//plain queue
	//Old Queue with lock
	if(queueType == 1)
		queue = new STLQueue<uint32_t>();
	//lockfree Queue for single producers
	else if(queueType == 2 && numProducer == 1)
		queue = new LockfreeSingleQueue<uint32_t>(queueSize);
	//lockfree Queue for multi producers
	else
		queue = new LockfreeMultiQueue<uint32_t>(queueSize);

}

QueueTest::~QueueTest() {
	for(uint32_t i=0; i<numProducer; i++){
		delete pusher[i];
	}
	delete popper;

	delete queue;
}

struct timespec QueueTest::runTest(uint32_t numOperations){
	struct timespec start;
	struct timespec stop;
	struct timespec ret;

	numOps = numOperations;

	//start timer resolution to 1 nano second
	clock_gettime(CLOCK_REALTIME, &start);

	//start Threads
	for(uint32_t i=0; i<numProducer; i++){
		pusher[i]->run(this);
	}
	popper->run(this);

	//wait for threads to finish
	for(uint32_t i=0; i<numProducer; i++){
		pusher[i]->join();
	}
	popper->join();

	//calculate difference
	clock_gettime(CLOCK_REALTIME, &stop);
	if ((stop.tv_nsec-start.tv_nsec)<0) {
		ret.tv_sec = stop.tv_sec-start.tv_sec-1;
		ret.tv_nsec = 1000000000 + stop.tv_nsec - start.tv_nsec;
	} else {
		ret.tv_sec = stop.tv_sec - start.tv_sec;
		ret.tv_nsec = stop.tv_nsec - start.tv_nsec;
	}

	//reset queue
	queue->reset();

	return ret;
}

void* QueueTest::pushFunc(void* ptr){
	QueueTest* qt = (QueueTest*)ptr;

	for(uint32_t i=1; i <= qt->numOps; i++){
		while(!qt->queue->push(i));
//		printf("pushed: %d\n",i);
	}

	return NULL;
}

void* QueueTest::popFunc(void* ptr){
	uint32_t element;
	uint32_t i;
	QueueTest* qt = (QueueTest*)ptr;

	for(i=1; i <= (qt->numOps) * (qt->numProducer); i++){
		while(!qt->queue->pop(&element));

//		fprintf(stderr,"popped: %d\n",element);
	}

	if(i != (qt->numOps) * (qt->numProducer) + 1)
		THROWEXCEPTION("wrong number of pop calls");

	return NULL;
}
