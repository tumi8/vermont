/*
 * Wrapper class around STL queue
 *
 * STLQueue.h
 *
 * Author: Simon Regnet <s-regnet@user.berlios.de>
 *
 */

#ifndef STLQUEUE_H
#define STLQUEUE_H

#include <queue>
#include "BaseQueue.h"
#include "Mutex.h"

template<class T>
class STLQueue : public BaseQueue<T>
{
	public:
		STLQueue() {};
		~STLQueue() {};

		/**
		 * enqueues a pointer to an element of type T in the queue
		 * @param element pointer to the element which will be enqueued.
		 * @return always true
		 */
		inline bool push(T element){
			lock.lock();
			queue.push(element);
			lock.unlock();

			return true;
		}

		/**
		 * returns the first pointer to an element of type T in queue
		 * and removes this pointer from the queue
		 * @return first pointer in the queue
		 */
		inline bool pop(T* element){
			lock.lock();
			*element = queue.front();
			queue.pop();
			lock.unlock();

			return true;
		}

	private:
		std::queue<T> queue;
		Mutex lock;
};

#endif
