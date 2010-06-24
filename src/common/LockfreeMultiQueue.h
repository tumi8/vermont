/*
 * Class implements a lock free thread safe queue for multiple
 * producers and multiple consumers.
 * IMPORTANT: use a type of pointer for template elements T
 * because NULL/0 will indicate free space in the internal array
 * of the queue where the elements are stored
 * This Code only works for 32 bit systems where pointers are
 * 32 bit of size. Problems are the Compare and Swap functions.
 *
 * LockfreeMultiQueue.h
 *
 * Author: Simon Regnet <s-regnet@user.berlios.de>
 *
 */

#ifndef LOCKFREEMULTIQUEUE_H
#define LOCKFREEMULTIQUEUE_H

#include <stdint.h>
#include <boost/static_assert.hpp>

#include "BaseQueue.h"

#define COMPARE_AND_SWAP(a,oldVal,newVal) __sync_bool_compare_and_swap(a,oldVal,newVal)

template<class T>
class LockfreeMultiQueue : public BaseQueue<T>
{
	public:
		LockfreeMultiQueue(uint32_t maxEntries):
			max(maxEntries),
			front(0), rear(0)
		{
			//template parameter may not be greater than 4 bytes
			BOOST_STATIC_ASSERT(sizeof(T) <= 4);

			buffer = new uint64_t[max];
			for(int i=0; i < max; i++){
				buffer[i] = 0;
			}
		}

		~LockfreeMultiQueue() {
			delete [] buffer;
		}

		/**
		 * enqueues an element of type T in the queue
		 * @param element which will be enqueued.
		 * @return always true
		 */
		inline bool push(T element){
			volatile uint64_t rearTmp, frontTmp;
			volatile uint64_t x;

			while(true){
				rearTmp = rear;
				x = buffer[rearTmp % max];
				frontTmp = front;
				if(rearTmp != rear)
					continue;

				if(rearTmp == front + max){
					if(valueOf(buffer[frontTmp % max]) != 0){
						if(frontTmp == front)
							return false;
					}
					COMPARE_AND_SWAP(&front, frontTmp, frontTmp + 1);
					continue;
				}

				if(valueOf(x) == 0){
					if(COMPARE_AND_SWAP(&buffer[rearTmp % max], x, makeEntry(element, refOf(x) +1))){
						COMPARE_AND_SWAP(&rear, rearTmp, rearTmp + 1);
						return true;
					}
				}
				else{
					if(valueOf(buffer[rearTmp % max]) != 0)
						COMPARE_AND_SWAP(&rear, rearTmp, rearTmp + 1);
				}
			}
		}

		/**
		 * returns the first element of type T in queue
		 * and removes it from the queue
		 * @return first pointer in the queue
		 */
		inline bool pop(T* element){
			volatile uint64_t frontTmp, rearTmp;
			volatile uint64_t x;

			while(true){
				frontTmp = front;
				x = buffer[frontTmp % max];
				rearTmp = rear;
				if(frontTmp != front)
					continue;

				if(frontTmp == rear){
					if(valueOf(buffer[rearTmp % max]) == 0){
						if(rearTmp == rear)
							return false;
					}
					COMPARE_AND_SWAP(&rear, rearTmp, rearTmp + 1);
					continue;
				}

				if(valueOf(x) != 0){
					if(COMPARE_AND_SWAP(&buffer[frontTmp % max], x, makeEntry(NULL, refOf(x)+1))){
						COMPARE_AND_SWAP(&front, frontTmp, frontTmp + 1);
						*element = (T)valueOf(x);
						return true;
					}
				}
				else{
					if(valueOf(buffer[frontTmp % max]) == 0)
						COMPARE_AND_SWAP(&front, frontTmp, frontTmp + 1);
				}
			}
		}

		/**
		 * resets the queue. Nulls all entries and sets
		 * the pointers to 0
		 */
		void reset(){
			for(int i=0; i < max; i++){
				buffer[i] = 0;
			}
			front = rear = 0;
		}

	private:


		/**
		 * Returns the higher 32 bits of the 64 bit argument.
		 * Should be the value of the entry.
		 * @entry 64 bit argument
		 * @return higher 32 bits of the 64 bit argument
		 */
		inline uint32_t valueOf(uint64_t entry){
		  return entry >> 32;
		}

		/**
		 * Returns the lower 32 bits of the 64 bit argument
		 * Should be the reference counter of the entry
		 * @entry 64 bit argument
		 * @return lower 32 bits of the 64 bit argument
		 */
		inline uint32_t refOf(uint64_t entry){
		    return 0xffffffff & entry;
		}

		/**
		 * Gets a value and a reference counter as argument
		 * and returns it as a 64 bit entry where value will
		 * be the higher 32 bit and ref the lower accordingly
		 *
		 * @value should be the value of the entry
		 * @ ref sould be the reference counter of the entry
		 * @return 64 bits combined by the two 32 bit arguments
		 */
		inline uint64_t makeEntry(T value, uint32_t ref){
		  uint64_t ret = (uint32_t)value;
		  ret = ret << 32;
		  ret += ref;
		  return ret;
		}

		volatile uint64_t front;
		volatile uint64_t rear;
		uint32_t max;
		uint64_t* buffer;

};

#endif
