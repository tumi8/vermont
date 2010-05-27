#ifndef SOURCE_H
#define SOURCE_H

/**
	@author Peter Baumann <siprbaum@users.berlios.de>
*/

#include "common/msg.h"
#include "common/Mutex.h"
#include "common/defs.h"
#include "common/CountingSemaphore.h"
#include "common/atomic_lock.h"


#include "core/Destination.h"
#include "core/Emitable.h"

template <typename T>
class Source
{
public:
	typedef T src_value_type;

	Source() : mutex(), connected(1), disconnectInProgress(false), syncLock(1), hasSuccessor(true), dest(NULL) { }
	virtual ~Source() { }

	virtual void connectTo(Destination<T>* destination)
	{
		mutex.lock();
		if (dest)
			THROWEXCEPTION("ERROR: already connected\n");
		dest = destination;
		connected.inc(1);
		atomic_release(&syncLock);
		mutex.unlock();
	}

	void connectToNothing()
	{
		mutex.lock();
		if (dest)
			THROWEXCEPTION("ERROR: already connected\n");
		hasSuccessor = false;
		dest = NULL;
		connected.inc(1);
		atomic_release(&syncLock);
		mutex.unlock();
	}

	virtual bool isConnected() const
	{
		/* FIXME: should this be inside a mutex to prevent race conditions?
		 *
		 *	  On the other hand, a caller must handle race conditions, because
		 *	  nothing prevents another thread to unconnect source from dest after
		 *	  returning true in this method
		 */
		return dest != NULL;
	}

	virtual void disconnect()
	{
		disconnectInProgress = true;
		mutex.lock();
		if (isConnected()) {
			// wait until function send has returned
			while (atomic_lock(&syncLock)) {
				timespec req;
				req.tv_sec = 0;
				req.tv_nsec = 1000000;
				nanosleep(&req, &req);
			}
			dest = NULL;
			connected.dec(1);
		}

		mutex.unlock();
		disconnectInProgress = false;
	}

	inline bool sleepUntilConnected()
	{
		// A counting semaphore is needed here,because otherwise there could
		// be a deadlock on disconnect and this method (if it is called inside a thread)
		bool retval = connected.dec(2);
		if (retval)
			connected.inc(2);
		return retval;
	}

	inline bool send(T t)
	{
		while (atomic_lock(&syncLock)) {
			if (!sleepUntilConnected()) {
				DPRINTF("Can't wait for connection, perhaps the program is shutting down?");
				return false;
			}
		}
		if (hasSuccessor) dest->receive(t);
		else {
			// we don't have a succeeding module, so clean up this data element
			t->removeReference();
		}
		atomic_release(&syncLock);

		return true;
	}

	// Subsequent modules that do not have
	// their own timer will be informed about the fact that
	// the queue is now running. It was added to inform
	// subsequent modules that
	// they can start using its predecessor's timer.
	inline void sendQueueRunningNotification() {
		bool c = false;
		mutex.lock();		
		if (isConnected()) {
			dest->notifyQueueRunning();
			c = true;
		}
		mutex.unlock();
		// Unless c == true something went badly wrong
		if (!c) THROWEXCEPTION("sendQueueRunningNotification() called although not connected.");
	}
	
	inline uint32_t atomicLock()
	{
		return atomic_lock(&syncLock);
	}

	inline void atomicRelease()
	{
		atomic_release(&syncLock);
	}

protected:
	Mutex mutex;
	CountingSemaphore connected;
	bool disconnectInProgress;

private:
	alock_t syncLock; /**< is locked when an element is sent to next module or no next module is available */
	bool hasSuccessor; /**< set to true if this module has a succeeding module */
	Destination<T>* dest;
};

template <> inline void Source<NullEmitable *>::sendQueueRunningNotification() { }

#endif

