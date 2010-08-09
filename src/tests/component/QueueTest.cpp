#include "QueueTest.h"
#include "common/Thread.h"

#include <stdio.h>

QueueTest::QueueTest(uint32_t queueType, uint32_t numProducer, uint32_t numConsumer, uint32_t queueSize):
	numProducer(numProducer),
	numConsumer(numConsumer),
	queueSize(queueSize),
	queue(NULL)
{
	if(numProducer > 20)
		THROWEXCEPTION("failed to create new thread");

	//creation of the threads
	for(uint32_t i=0; i<numProducer; i++){
		pusher[i] = new Thread(QueueTest::pushFunc);
	}
	for(uint32_t i=0; i<numConsumer; i++){
		popper[i] = new Thread(QueueTest::popFunc);
	}

	//creation of the queue
	switch(queueType){
		case 1:
			queue = new STLQueue<uint32_t>();
			break;
		case 2:
			queue = new LockfreeSingleQueueBasic<uint32_t>(queueSize);
			break;
		case 3:
			queue = new LockfreeSingleQueueCacheOpt<uint32_t>(queueSize);
			break;
		case 4:
			queue = new LockfreeMultiQueue<uint32_t>(queueSize);
			break;
		default:
			THROWEXCEPTION("wrong queue type");
	}
}

QueueTest::~QueueTest() {
	for(uint32_t i=0; i<numProducer; i++){
		delete pusher[i];
	}
	for(uint32_t i=0; i<numConsumer; i++){
		delete popper[i];
	}

	delete queue;
}

returnClass QueueTest::runTest(uint32_t numOperations){
	returnClass ret;
	struct timespec start;
	struct timespec stop;

	numOps = numOperations;
	if(numOps % 10 != 0)
		THROWEXCEPTION("numOps is not valid because of batchUpdate");

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
		uint32_t* ptr = (uint32_t*)(pusher[i]->join());
		ret.full += *ptr;
		delete ptr;
	}
	for(uint32_t i=0; i<numConsumer; i++){
		uint32_t* ptr = (uint32_t*)(popper[i]->join());
		ret.empty += *ptr;
		delete ptr;
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

	//reset queue
	queue->reset();

	return ret;
}

void* QueueTest::pushFunc(void* ptr){
	QueueTest* qt = (QueueTest*)ptr;
	uint32_t ops = qt->numOps;
	BaseQueue<uint32_t>* q = qt->queue;
	uint32_t fullCount = 0;

	for(uint32_t i=1; i <= ops; i++){
		while(!q->push(i)){
			fullCount++;
			//printf("fullCount: %d",fullCount);
		}
//		printf("pushed: %d\n",i);
	}

	return new uint32_t(fullCount);
}

void* QueueTest::popFunc(void* ptr){
	uint32_t element;
	uint32_t i;
	QueueTest* qt = (QueueTest*)ptr;
	uint32_t ops = ((qt->numOps) * (qt->numProducer)) / qt->numConsumer;
	BaseQueue<uint32_t>* q = qt->queue;
	uint32_t emptyCount = 0;

	for(i=1; i <= ops; i++){
		while(!q->pop(&element)){
			emptyCount++;
			//printf("emptyCount: %d",emptyCount);
		}
//		fprintf(stderr,"popped: %d\n",element);
	}

	if(i != ops + 1)
		THROWEXCEPTION("wrong number of pop calls");

	return new uint32_t(emptyCount);
}
