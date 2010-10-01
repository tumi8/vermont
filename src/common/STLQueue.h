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
		STLQueue() {}
		~STLQueue() {};

		/**
		 * enqueues an element of type T in the queue
		 * @param element which will be enqueued.
		 * @return always true
		 */
		bool push(T element){
			lock.lock();
			queue.push(element);
			lock.unlock();

			return true;
		}

		/**
		 * dequeues the first element of type T in queue
		 * and removes it from the queue
		 * @param element which will be dequeued
		 * @return false if no element in queue
		 */
		bool pop(T* element){
			lock.lock();

			if(queue.empty()){
				lock.unlock();
				return false;
			}

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
