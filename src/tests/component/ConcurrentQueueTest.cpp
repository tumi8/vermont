#include "ConcurrentQueueTest.h"
#include "common/Thread.h"

#include <stdio.h>

ConcurrentQueueTest::ConcurrentQueueTest(uint32_t queueType, uint32_t numProducer, uint32_t numConsumer, uint32_t queueSize, uint32_t timeoutLength):
	numProducer(numProducer),
	numConsumer(numConsumer),
	queueSize(queueSize),
	timeoutLength(timeoutLength),
	queue(NULL)
{
	if(numProducer > 20)
		THROWEXCEPTION("failed to create new thread");

	//creation of the threads
	for(uint32_t i=0; i<numProducer; i++){
		pusher[i] = new Thread(ConcurrentQueueTest::pushFunc);
	}
	for(uint32_t i=0; i<numConsumer; i++){
		popper[i] = new Thread(ConcurrentQueueTest::popFunc);
	}

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
			if(numProducer != 1 || numConsumer != 1)
				THROWEXCEPTION("Queue can only handle 1 producer/consumer");
			queue->deleteQueue();
			queue->setQueue(new LockfreeSingleQueueBasic<uint32_t>(queueSize));
			break;
		case 3:
			if(numProducer != 1 || numConsumer != 1)
				THROWEXCEPTION("Queue can only handle 1 producer/consumer");
			delete queue->queueImp;
			queue->setQueue(new LockfreeSingleQueueCacheOpt<uint32_t>(queueSize));
			break;
		case 4:
			delete queue->queueImp;
			queue->setQueue(new LockfreeMultiQueue<uint32_t>(queueSize));
			break;
		default:
			THROWEXCEPTION("wrong QueueType");
	}

}

ConcurrentQueueTest::~ConcurrentQueueTest() {
	for(uint32_t i=0; i<numProducer; i++){
		delete pusher[i];
	}
	for(uint32_t i=0; i<numConsumer; i++){
		delete popper[i];
	}

	delete queue;
}

returnClassCon ConcurrentQueueTest::runTest(uint32_t numOperations){
	returnClassCon ret;
	struct timespec start;
	struct timespec stop;

	numOps = numOperations;

	//start timer resolution to 1 nano second
	clock_gettime(CLOCK_REALTIME, &start);

	//start Threads
	for(uint32_t i=0; i<numProducer; i++){
		pusher[i]->run(this);
	}
	for(uint32_t i=0; i<numConsumer; i++){
		popper[i]->run(this);
	}

	//wait for threads to finish
	for(uint32_t i=0; i<numProducer; i++){
		pusher[i]->join();

	}
	for(uint32_t i=0; i<numConsumer; i++){
		popper[i]->join();
	}

	//calculate difference
	clock_gettime(CLOCK_REALTIME, &stop);
	if ((stop.tv_nsec-start.tv_nsec)<0) {
		ret.timespent.tv_sec = stop.tv_sec-start.tv_sec-1;
		ret.timespent.tv_nsec = 1000000000 + stop.tv_nsec - start.tv_nsec;
	} else {
		ret.timespent.tv_sec = stop.tv_sec - start.tv_sec;
		ret.timespent.tv_nsec = stop.tv_nsec - start.tv_nsec;
	}

	ret.full = queue->getFullCount();
	ret.empty = queue->getEmptyCount();

	//reset queue
	queue->reset();

	return ret;
}

void* ConcurrentQueueTest::pushFunc(void* ptr){
	ConcurrentQueueTest* qt = (ConcurrentQueueTest*)ptr;
	uint32_t ops = qt->numOps;
	BaseConcurrentQueue<uint32_t>* cq = qt->queue;

	for(uint32_t i=1; i <= ops; i++){
		cq->push(i);
//		printf("pushed: %d\n",i);
	}

	return NULL;
}

void* ConcurrentQueueTest::popFunc(void* ptr){
	uint32_t element;
	uint32_t i;
	ConcurrentQueueTest* qt = (ConcurrentQueueTest*)ptr;
	uint32_t ops = ((qt->numOps) * (qt->numProducer)) / qt->numConsumer;
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
