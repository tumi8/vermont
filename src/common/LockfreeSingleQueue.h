/*
 * Class implements a lock free thread safe queue for
 * single producer and single consumer constellation.
 * It is described in "A Lock-Free, Cache-Efficient Multi-Core
 * Synchronization Mechanism for Line-Rate Network Traffic
 * Monitoring" by Patrick P. C. Lee.
 *
 * LockfreeSingleQueue.h
 *
 * Author: Simon Regnet <s-regnet@user.berlios.de>
 *
 */

#ifndef LOCKFREESINGLEQUEUE_H
#define LOCKFREESINGLEQUEUE_H

#include <stdlib.h>
#include <stdint.h>

#include "BaseQueue.h"
#include "osdep/linux/sysinfo.h"


template<class T>
class LockfreeSingleQueue : public BaseQueue<T>
{
public:

	/**
	 * initializes the queue and sets the maximum number
	 * of enqueued elements
	 * @param maxEntries maximum number of enqueued elements
	 */
	LockfreeSingleQueue(uint32_t maxEntries){
		uint32_t clsize = getCachelineSize();
		if(clsize < 3 * sizeof(uint32_t))
			THROWEXCEPTION("Error: Systems cache-line size is too small");
		void* tmp;

		/*get space for 4 cache lines, one padding cacheline in front of it*/
		if(posix_memalign(&tmp, clsize, clsize*4) != 0)
			THROWEXCEPTION("Error: posix_memalign()");

		/*shared control variables*/
		read = (uint32_t*)tmp;
		write = read + 1;
		*read = *write = 0;

		/*consumer’s local variables*/
		localWrite = (uint32_t*)tmp + (clsize/sizeof(uint32_t*));
		nextRead = localWrite + 1;
		rBatch = nextRead + 1;
		*localWrite = *nextRead = *rBatch = 0;

		/*producer’s local variables*/
		localRead = (uint32_t*)tmp + 2*(clsize/sizeof(uint32_t*));
		nextWrite = localRead + 1;
		wBatch = nextWrite + 1;
		*localRead = *nextWrite = *wBatch = 0;

		/*constants*/
		max = (uint32_t*)tmp + 3*(clsize/sizeof(uint32_t*));
		batchSize = max + 1;
		*max = maxEntries+1;
		*batchSize = 10;

		buffer = new T[*max];
	}

	~LockfreeSingleQueue() {
		free((void*)read);
		delete [] buffer;
	}

	/**
	 * enqueues an element of type T in the queue
	 * @param element element which will be enqueued.
	 * @return false if queue is full, otherwise true
	 */
	inline bool push(T element){
		uint32_t afterNextWrite = next(*nextWrite);

		if(afterNextWrite == *localRead){
			if(afterNextWrite == *read)
				return false;
			*localRead = *read;
		}

		buffer[*nextWrite] = element;
		*nextWrite = afterNextWrite;

		(*wBatch)++;
		if(*wBatch >= *batchSize){
			*write = *nextWrite;
			*wBatch = 0;
		}

		return true;
	}

	/**
	 * returns the first element of type T in queue
	 * and removes it from the queue
	 * @param element pointer where dequeued element will be stored
	 * @return false if no element to be dequeued, otherwise true
	 */
	inline bool pop(T* element){
		if(*nextRead == *localWrite){
			if(*nextRead == *write)
				return false;
			*localWrite = *write;
		}

		*element = buffer[*nextRead];
		*nextRead = next(*nextRead);

		(*rBatch)++;
		if(*rBatch >= *batchSize){
			*read = *nextRead;
			*rBatch = 0;
		}

		return true;
	}

	/**
	 * updates read and write so that no elements stick in the queue
	 */
	inline void batchUpdate(){
		*write = *nextWrite;
		*read = *nextRead;
		*rBatch = *wBatch = 0;
	}

	/**
	 * restarts the queue and sets the internal
	 * pointers to the initial values
	 */
	void reset(){
		*read = *write = 0;
		*localWrite = *nextRead = 0;
		*localRead = *nextWrite = 0;
		*rBatch = *wBatch = 0;
	}
private:

	/**
	 * returns next array position
	 * @param index current array position
	 * @return consecutive array position
	 */
	inline uint32_t next(uint32_t index){
		return (++index % *max);
	}

	/*shared control variables*/
	uint32_t* read;
	uint32_t* write;
	/*consumer’s local variables*/
	uint32_t* localWrite;
	uint32_t* nextRead;
	uint32_t* rBatch;
	/*producer’s local variables*/
	uint32_t* localRead;
	uint32_t* nextWrite;
	uint32_t* wBatch;
	/*constants*/
	uint32_t* max;
	uint32_t* batchSize;
	T* buffer;
};

#endif
