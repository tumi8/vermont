/*
 * Abstract base class for several queue implementations
 *
 * Make sure that size of T is 4 bytes on 32 bit operating systems
 * and 8 bytes on 64 bit operating systems accordingly.
 * This is more efficient with STL and LOCKFREE_SINGLE queues and
 * even mandatory for LOCKFREE_MULTI queues
 *
 * BaseQueue.h
 *
 * Author: Simon Regnet <s-regnet@user.berlios.de>
 *
 */

#ifndef BASEQUEUE_H
#define BASEQUEUE_H

template<class T>
class BaseQueue
{
	public:
		BaseQueue() {}
		virtual ~BaseQueue() {}
		virtual bool push(T) = 0;
		virtual bool pop(T*) = 0;
};

#endif
