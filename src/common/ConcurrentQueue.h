/*
 * Thread-safe (concurrent) queue implementation
 * The implementation of the BaseQueue makes it thread-safe
 *
 * Make sure that size of T is 4 bytes on 32 bit operating systems
 * and 8 bytes on 64 bit operating systems accordingly when using
 * QueueType LOCKFREE_MULTI
 *
 * Author: Michael Drueing <michael@drueing.de>
 *         Simon Regnet <s-regnet@user.berlios.de>
 *
 */

#ifndef CONCURRENT_QUEUE_H
#define CONCURRENT_QUEUE_H

#include <string>
#include "BaseQueue.h"
#include "STLQueue.h"
#include "LockfreeSingleQueue.h"
#include "LockfreeMultiQueue.h"
#include "msg.h"
#include "osdep/linux/sysinfo.h"
#include <time.h>

/**
 * types of queues
 */
enum QUEUETYPES {
	STL, SINGLE, MULTI
};

template<class T>
class ConcurrentQueue {
public:
	/**
	 * default queue size
	 */
	static const uint32_t DEFAULT_QUEUE_SIZE = 1000;

	/**
	 * default queue size
	 */
	static const uint32_t DEFAULT_TIMEOUT = 51;

	/**
	 * initializes the queue
	 * @param qType STL | SINGLE | MULTI
	 * @param maxEntries maximum number of enqueued items
	 * @param spinLockTimeout timeout in ns after failed operation
	 */
	ConcurrentQueue(uint32_t qType = MULTI, uint32_t maxEntries =
			DEFAULT_QUEUE_SIZE, uint32_t spinLockTimeout = DEFAULT_TIMEOUT) {
		switch (qType) {
		case STL:
			this->queueImp = new STLQueue<T> ();
			break;
		case SINGLE:
			this->queueImp = new LockfreeSingleQueue<T> (maxEntries);
			break;
		case MULTI:
			this->queueImp = new LockfreeMultiQueue<T> (maxEntries);
			break;
		default:
			THROWEXCEPTION("Unknown Queue Type");
		}

		shutdownFlag = false;

		//initialize variable in cachelines
		void* tmp;
		uint32_t clsize = getCachelineSize();
		if (2 * sizeof(uint32_t) + sizeof(struct timespec)
				+ sizeof(BaseQueue<T>**) > clsize)
			THROWEXCEPTION("Error: Cacheline Size is not big enough");

		if (posix_memalign(&tmp, clsize, 2 * clsize) != 0)
			THROWEXCEPTION("Error: posix_memalign()");

		//producers variables
		pushedCount = ((uint32_t*) tmp);
		spinLockTimeoutProducer = (struct timespec*) (pushedCount + 1);
		queueImpProducer = (BaseQueue<T>**) (spinLockTimeoutProducer + 1);
		*pushedCount = 0;
		spinLockTimeoutProducer->tv_sec = 0;
		spinLockTimeoutProducer->tv_nsec = spinLockTimeout;
		*queueImpProducer = this->queueImp;

		//consumer variables
		poppedCount = ((uint32_t*) tmp) + (clsize / sizeof(uint32_t*));
		spinLockTimeoutConsumer = (struct timespec*) (poppedCount + 1);
		queueImpConsumer = (BaseQueue<T>**) (spinLockTimeoutConsumer + 1);
		numSlots = (uint32_t*) (queueImpConsumer + 1);
		*poppedCount = 0;
		spinLockTimeoutConsumer->tv_sec = 0;
		spinLockTimeoutConsumer->tv_nsec = spinLockTimeout;
		*queueImpConsumer = this->queueImp;
		*numSlots = maxEntries;
	}

	~ConcurrentQueue() {
		if (getCount() != 0) {
			msg(MSG_DEBUG, "WARNING: freeing non-empty queue - got count: %d",
					getCount());
		}
		free((void*) pushedCount);
	}

	/**
	 * sets the name of the owner
	 * @param name name of the owner
	 */
	void setOwner(std::string name) {
		ownerName = name;
	}

	/**
	 * enqueue an element
	 * @param t element to be enqueued
	 */
	inline void push(T t) {
		while (!(*(this->queueImpProducer))->push(t)) {
			(*(this->queueImpProducer))->batchUpdate();

			nanosleep(spinLockTimeoutProducer, NULL);

			if (shutdownFlag)
				return;
		}

		__sync_add_and_fetch(pushedCount, 1);

	}

	/**
	 * dequeues an element of the queue
	 * @param res pointer where the dequeued element will be stored
	 * @return false if ConcurrentQueue was shutdown during the operation, true if successfull
	 */
	inline bool pop(T* res) {
		while (!(*(this->queueImpConsumer))->pop(res)) {
			(*(this->queueImpConsumer))->batchUpdate();

			nanosleep(spinLockTimeoutConsumer, NULL);

			if (spinLockTimeoutConsumer->tv_nsec * 2 < 51 * (*numSlots))
				spinLockTimeoutConsumer->tv_nsec *= 2;

			if (shutdownFlag)
				return false;
		}

		spinLockTimeoutConsumer->tv_nsec = 51;

		(*poppedCount)++;

		return true;
	}

	/**
	 *  tries to enqueue an element. If the the operation can't be done
	 *  within the given timeout, the operation fails.
	 *  @param timeout timeout till the operation has to be finished
	 *  @param res pointer where the dequeued element will be stored
	 *  @return true if element could be dequeued, false otherwise
	 */
	inline bool popAbs(const struct timespec& timeout, T *res) {
		if (!(*(this->queueImpConsumer))->pop(res)) {
			(*(this->queueImpConsumer))->batchUpdate();

			nanosleep(&timeout, NULL);

			if (!(*(this->queueImpConsumer))->pop(res))
				return false;
		}

		spinLockTimeoutConsumer->tv_nsec = 51;

		(*poppedCount)++;

		return true;
	}

	/**
	 * @return number of currently inserted elements
	 */
	inline uint32_t getCount() const {
		return *pushedCount - *poppedCount;
	}

	/**
	 * restarts the queue
	 */
	inline void restart() {
		queueImp->reset();
	}

	/**
	 * after calling this function, queue will not block again but return
	 * all functions with an error
	 * (useful for shutdown of this instance)
	 */
	void notifyShutdown() {
		shutdownFlag = true;
	}

protected:
	//producer variables
	uint32_t* pushedCount;
	struct timespec* spinLockTimeoutProducer;
	BaseQueue<T>** queueImpProducer;

	//consumer variables
	uint32_t* poppedCount;
	struct timespec* spinLockTimeoutConsumer;
	BaseQueue<T>** queueImpConsumer;
	uint32_t* numSlots;

	BaseQueue<T>* queueImp;
	std::string ownerName;
	bool shutdownFlag;
};

#endif

