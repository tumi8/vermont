#include "QueueTest.h"
#include "common/Thread.h"

#include <stdio.h>

QueueTest::QueueTest(uint32_t queueType, uint32_t numProducer, uint32_t queueSize, uint32_t timeoutLength):
	numProducer(numProducer),
	queueSize(queueSize),
	timeoutLength(timeoutLength),
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
	switch(queueType / 10){
		case 1:
			queue = new ConcurrentQueue<uint32_t>(STL, queueSize);
			break;
		case 2:
			queue = new ConcurrentQueueCond<uint32_t>(STL, queueSize, timeoutLength);
			break;
		case 3:
			queue = new ConcurrentQueueSpinlock<uint32_t>(STL, queueSize, timeoutLength);
			break;
		default:
			THROWEXCEPTION("wrong ConcurrentQueue");
	}

	switch(queueType % 10){
		case 1:
			break;
		case 2:
			if(numProducer != 1)
				THROWEXCEPTION("Queue can only handle 1 producer");
			delete queue->queueImp;
			queue->queueImp = new LockfreeSingleQueueBasic<uint32_t>(queueSize);
			break;
		case 3:
			if(numProducer != 1)
				THROWEXCEPTION("Queue can only handle 1 producer");
			delete queue->queueImp;
			queue->queueImp = new LockfreeSingleQueueCacheOpt<uint32_t>(queueSize);
			break;
		case 4:
			delete queue->queueImp;
			queue->queueImp = new LockfreeMultiQueue<uint32_t>(queueSize);
			break;
		default:
			THROWEXCEPTION("wrong QueueType");
	}

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
	uint32_t ops = qt->numOps;
	BaseConcurrentQueue<uint32_t>* cq = qt->queue;

	for(uint32_t i=1; i <= ops; i++){
		cq->push(i);
//		printf("pushed: %d\n",i);
	}

	return NULL;
}

void* QueueTest::popFunc(void* ptr){
	uint32_t element;
	uint32_t i;
	QueueTest* qt = (QueueTest*)ptr;
	uint32_t ops = (qt->numOps) * (qt->numProducer);
	BaseConcurrentQueue<uint32_t>* cq = qt->queue;

	for(i=1; i <= ops; i++){
		if(!cq->pop(&element))
			printf("pop failed in QueueTest.cpp\n");

//		fprintf(stderr,"popped: %d\n",element);
	}

	if(i != ops + 1)
		THROWEXCEPTION("wrong number of pop calls");

	return NULL;
}
