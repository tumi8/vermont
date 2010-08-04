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
class LockfreeSingleQueueCacheOpt : public BaseQueue<T>
{
public:
	LockfreeSingleQueueCacheOpt(int maxEntries){
		uint32_t clsize = getCachelineSize();
		void* tmp;

		/*get space for 4 cache lines*/
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

	~LockfreeSingleQueueCacheOpt() {
		free((void*)read);
		delete [] buffer;
	}

	/**
	 * enqueues an element of type T in the queue
	 * @param element which will be enqueued.
	 * @return always true
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
	 * @return first pointer in the queue
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
	 * restarts the queue and sets the internal
	 * pointers to initial values
	 */
	void reset(){
		*read = *write = 0;
		*localWrite = *nextRead = 0;
		*localRead = *nextWrite = 0;
		*rBatch = *wBatch = 0;
	}
private:

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


template<class T>
class LockfreeSingleQueueBasic : public BaseQueue<T>
{
	public:
		LockfreeSingleQueueBasic(uint32_t maxEntries):
			maxEntries(maxEntries+1),
			write(0),
			read(0)
		{
			buffer = new T[this->maxEntries];
		}

		~LockfreeSingleQueueBasic() {
			delete [] buffer;
		}

		/**
		 * enqueues an element of type T in the queue
		 * @param element which will be enqueued.
		 * @return always true
		 */
		inline bool push(T element){
			if(next(write) == read)
				return false;

			buffer[write] = element;
			write = next(write);

			return true;
		}

		/**
		 * returns the first element of type T in queue
		 * and removes it from the queue
		 * @return first pointer in the queue
		 */
		inline bool pop(T* element){
			if(read == write)
				return false;

			*element = buffer[read];
			read = next(read);

			return true;
		}

		/**
		 * restarts the queue and sets the internal
		 * pointers to initial values
		 */
		void restart(){
			read = write = 0;
		}

	private:

		inline uint32_t next(uint32_t index){
			return ++index % maxEntries;
		}

		uint32_t maxEntries;
		T* buffer;
		uint32_t write;
		uint32_t read;

};


//cache line size
#define CACHE_LINE 64

template<class T>
class LockfreeSingleQueueCacheOptLocal : public BaseQueue<T>
{
	public:
		LockfreeSingleQueueCacheOptLocal(uint32_t maxEntries):
			maxEntries(maxEntries+1),
			write(0), read(0),
			localWrite(0), nextRead(0),
			localRead(0), nextWrite(0)
		{
			buffer = new T[this->maxEntries];
		}

		~LockfreeSingleQueueCacheOptLocal() {
			delete [] buffer;
		}

		/**
		 * enqueues an element of type T in the queue
		 * @param element which will be enqueued.
		 * @return always true
		 */
		inline bool push(T element){
			uint32_t afterNextWrite = next(nextWrite);

			if(afterNextWrite == localRead){
				if(afterNextWrite == read)
					return false;
				localRead = read;
			}

			buffer[nextWrite] = element;
			nextWrite = afterNextWrite;
			//TODO maybe add batch update functionality here
			write = nextWrite;

			return true;
		}

		/**
		 * returns the first element of type T in queue
		 * and removes it from the queue
		 * @return first pointer in the queue
		 */
		inline bool pop(T* element){
			if(nextRead == localWrite){
				if(nextRead == write)
					return false;
				localWrite = write;
			}

			*element = buffer[nextRead];
			nextRead = next(nextRead);
			//TODO maybe add batch update functionality here
			read = nextRead;

			return true;
		}

		/**
		 * restarts the queue and sets the internal
		 * pointers to initial values
		 */
		void restart(){
			read = write = 0;
			localWrite = nextRead = 0;
			localRead = nextWrite = 0;
		}

	private:

		inline uint32_t next(uint32_t index){
			return (++index % maxEntries);
		}

		char pad0[CACHE_LINE];
		/*shared control variables*/
		volatile uint32_t read;
		volatile uint32_t write;
		char pad1[CACHE_LINE - 2*sizeof(uint32_t)];
		/*consumer’s local variables*/
		uint32_t localWrite;
		uint32_t nextRead;
		char pad2[CACHE_LINE - 2*sizeof(uint32_t)];
		/*producer’s local variables*/
		uint32_t localRead;
		uint32_t nextWrite;
		char pad3[CACHE_LINE - 2*sizeof(uint32_t)];
		/*constants*/
		uint32_t maxEntries;
		T* buffer;
		char pad4[CACHE_LINE - sizeof(uint32_t) - sizeof(T*)];

};


#endif
