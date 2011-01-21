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

#include "core/Module.h"
#include "core/Destination.h"
#include "core/Emitable.h"

#include <vector>

using namespace std;


template <typename T>
class Source
{
protected:
	Mutex mutex;
	CountingSemaphore connected;
	bool disconnectInProgress;

private:
	alock_t syncLock; /**< is locked when an element is sent to next module or no next module is available */
	bool hasSuccessor; /**< set to true if this module has a succeeding module */
	vector<Destination<T>*> destinations;
	uint32_t destinationSize;
	/**
	 * This vector stores optional modules that do not need to have correct element types for reception.
	 * If they are allowed, no elements will be sent there through the normal module graph. This module
	 * instance has to care for itself to send elements there (as well as ensure correct element types)
	 */
	vector<Module*> optDestinations;
	uint32_t optDestinationSize;
	bool allowOptionalModules;

public:
	typedef T src_value_type;

	Source()
		: mutex(),
		  connected(1),
		  disconnectInProgress(false),
		  syncLock(1),
		  hasSuccessor(true),
		  destinationSize(0),
		  optDestinationSize(0),
		  allowOptionalModules(false)
	{ }

	virtual ~Source() { }

	virtual void connectTo(Destination<T>* destination)
	{
		mutex.lock();
		destinations.push_back(destination);
		destinationSize++;
		connected.inc(1);
		atomic_release(&syncLock);
		mutex.unlock();
	}


	virtual void connectToOptional(Module* destination)
	{
		if (!allowOptionalModules)
			THROWEXCEPTION("Source: internal error, optional error are not allowed for this module, but one would have been inserted nevertheless!");

		mutex.lock();
		optDestinations.push_back(destination);
		optDestinationSize++;
		mutex.unlock();
	}

	void setAllowOptionalModules(bool b)
	{
		allowOptionalModules = b;
	}

	bool optionalModulesAllowed()
	{
		return allowOptionalModules;
	}

	void connectToNothing()
	{
		mutex.lock();
		if (destinationSize>0 || optDestinationSize>0)
			THROWEXCEPTION("ERROR: already connected\n");
		hasSuccessor = false;
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
		return destinationSize>0;
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
			destinations.clear();
			optDestinations.clear();
			destinationSize = 0;
			optDestinationSize = 0;
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

	inline bool send(T t){
		return send(t, -1);
	}

	inline bool send(T t, int id)
	{
		while (atomic_lock(&syncLock)) {
			if (!sleepUntilConnected()) {
				DPRINTF("Can't wait for connection, perhaps the program is shutting down?");
				return false;
			}
		}
		if (destinationSize>0) {
			if (id < 0) {
				if (destinationSize>1) t->addReference(destinationSize-1);
				for (uint32_t i=0; i<destinationSize; i++) {
					destinations[i]->receive(t);
				}
			} else {
				destinations[id]->receive(t);
			}
		}
		else {
			// we don't have a succeeding module, so clean up this data element
			t->removeReference();
		}
		atomic_release(&syncLock);

		return true;
	}

	Destination<T>* getSucceedingModuleInstance(int id)
	{
		if (id<0 || id>=(int)destinationSize) THROWEXCEPTION("Source::getSucceedingModuleInstance() got invalid id (%d)", id);
		return destinations[id];
	}

	int getSucceedingModuleCount()
	{
		return destinationSize;
	}

	Module* getSucceedingOptModuleInstance(int id)
	{
		if (id<0 || id>=(int)optDestinationSize) THROWEXCEPTION("Source::getSucceedingOptModuleInstance() got invalid id (%d)", id);
		return optDestinations[id];
	}

	int getSucceedingOptModuleCount()
	{
		return optDestinationSize;
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
			for (uint32_t i=0; i<destinationSize; i++) {
				destinations[i]->notifyQueueRunning();
			}
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
};

template <> inline void Source<NullEmitable *>::sendQueueRunningNotification() { }

#endif

