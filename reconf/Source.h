#ifndef SOURCE_H
#define SOURCE_H

/**
	@author Peter Baumann <siprbaum@users.berlios.de>
*/

#include "common/msg.h"
#include "common/Mutex.h"
#include "common/CountingSemaphore.h"


#include "reconf/Destination.h"
#include "reconf/Emitable.h"

class BaseSource {
public:
	 virtual ~BaseSource() { }

	virtual void connectTo(BaseDestination* dest) = 0;
	virtual void disconnect() = 0;

	virtual bool isConnected() const = 0;
};

template <class T>
class Source : public virtual BaseSource
{
public:
	Source() : mutex(), connected(1), dest(NULL) { }
	virtual ~Source() { }

	virtual void connectTo(BaseDestination* destination)
	{
		Destination<T>* d = dynamic_cast< Destination<T>* >(destination);
		if (!d) {
			THROWEXCEPTION("ERROR: Can't connect to this .... thing?");
		}
		this->connectTo(d);
	}

	virtual void connectTo(Destination<T>* destination)
	{
		mutex.lock();
		if (dest)
			THROWEXCEPTION("ERROR: already connected\n");
		dest = destination;
		connected.inc(1);
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
		mutex.lock();
		if (isConnected()) {
			dest = NULL;
			connected.dec(1);
		}
		mutex.unlock();
	}

	inline bool sleepUntilConnected()
	{
		// A counting semaphore is needed here,because otherwise there could
		// be a deadlock on disconnect and this method (if it is called inside a thread)
		bool retval = connected.dec(2);
		connected.inc(2);
		return retval;
	}
	
	inline bool send(T t)
	{
		Destination<T>* d;
		while ((d = dest) == NULL) {
			if (!sleepUntilConnected()) {
				DPRINTF("Can't wait for connection, perhaps the program is shutting down?");
				return false;
			}
		}
		
		d->receive(t);
		return true;
	}
	
protected:
	Mutex mutex;
	CountingSemaphore connected;
	Destination<T>* dest;
};

#endif

