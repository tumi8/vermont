#ifndef SOURCE_H
#define SOURCE_H

/**
	@author Peter Baumann <siprbaum@users.berlios.de>
*/

#include "common/Mutex.h"

#include "reconf/Destination.h"
#include "reconf/Emitable.h"

#include <stdexcept>

class BaseSource {
public:
	 virtual ~BaseSource() { }

	virtual void connectTo(BaseDestination* dest) = 0;
	virtual void disconnect() = 0;

	virtual bool isConnected() const = 0;
};

template <class T>
class Source : public BaseSource
{
public:
	Source() : mutex(), dest(NULL) { }
	virtual ~Source() { }

	virtual void connectTo(BaseDestination* destination)
	{
		Destination<T>* d = dynamic_cast< Destination<T>* >(destination);
		if (!d) {
			throw std::runtime_error("ERROR: Can't connect to this\n");
		}
		this->connectTo(d);
	}

	virtual void connectTo(Destination<T>* destination)
	{
		mutex.lock();
		if (dest)
			throw std::runtime_error("ERROR: already connected\n");
		dest = destination;
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
		dest = NULL;
		mutex.unlock();
	}

protected:
	Mutex mutex;
	Destination<T>* dest;
};

#endif

