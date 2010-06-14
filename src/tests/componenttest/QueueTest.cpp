#include "QueueTest.h"
#include "common/Thread.h"

#include <stdio.h>

QueueTest::QueueTest(int queueType, int numProducer, int queueSize):
	queueType(queueType),
	numProducer(numProducer),
	queueSize(queueSize),
	queue(NULL)
{
	if(numProducer > 20)
		THROWEXCEPTION("failed to create new thread");

	for(int i=0; i<numProducer; i++){
		pusher[i] = new Thread(QueueTest::pushFunc);
	}
	popper = new Thread(QueueTest::popFunc);
}

QueueTest::~QueueTest() {
	for(int i=0; i<numProducer; i++){
		delete pusher[i];
	}
	delete popper;
}

struct timespec QueueTest::runTest(int numOperations){
	struct timespec start;
	struct timespec stop;
	struct timespec ret;

	numOps = numOperations;

	//create needed queue
	//Old Queue with lock
	if(queueType == 1)
		queue = new ConcurrentQueue<int>(ConcurrentQueue<int>::STL, queueSize);
	//lockfree Queue for single producers
	else if(queueType == 2 && numProducer == 1)
		queue = new ConcurrentQueue<int>(ConcurrentQueue<int>::STL, queueSize);
	//lockfree Queue for single producers
	else
		queue = new ConcurrentQueue<int>(ConcurrentQueue<int>::STL, queueSize);

	//resolution to 1 nano second
	clock_gettime(CLOCK_REALTIME, &start);

	//start Threads
	for(int i=0; i<numProducer; i++){
		pusher[i]->run(this);
	}
	popper->run(this);

	//wait for threads to finish
	for(int i=0; i<numProducer; i++){
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

	if(queue != NULL)
		delete queue;

	return ret;
}

void* QueueTest::pushFunc(void* ptr){
	QueueTest* qt = (QueueTest*)ptr;

	for(int i=0; i < qt->numOps; i++){
		qt->queue->push(i);
		printf("pushed: %d\n",i);
	}

	return NULL;
}

void* QueueTest::popFunc(void* ptr){
	int element;
	int i;
	QueueTest* qt = (QueueTest*)ptr;

	for(i=0; i < (qt->numOps) * (qt->numProducer); i++){
		qt->queue->pop(&element);
		fprintf(stderr,"popped: %d\n",element);
	}

	if(i != (qt->numOps) * (qt->numProducer))
		THROWEXCEPTION("failed to create new thread");

	return NULL;
}
