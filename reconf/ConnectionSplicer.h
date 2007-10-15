#ifndef PACKETCONNECTIONSPLICER_H
#define PACKETCONNECTIONSPLICER_H

/**
	@author Peter Baumann <siprbaum@users.berlios.de>
*/

#include "common/msg.h"

#include "reconf/Adapter.h"
#include "reconf/Destination.h"
#include "reconf/Source.h"

#include "sampler/Packet.h"

#include <vector>

template <class T>
class ConnectionSplicer
	: public Adapter<T>
{
public:
	ConnectionSplicer(): size(0) { }

	virtual ~ConnectionSplicer() { }

	virtual void connectTo(BaseDestination* destination)
	{
		Destination<T>* d = dynamic_cast< Destination<T>* >(destination);
		if (!d) {
			THROWEXCEPTION("ERROR: Can't connect to this .... thing?");
		}
		this->connectTo(d);
	}
	
	virtual void connectTo(Destination<T>* d)
	{
		Source<T>::mutex.lock();

		if (size >= capacity)
			THROWEXCEPTION("ConnectionSplicer: can't add another destination\n");

		destinations[size++] = d;
		Source<T>::connected.inc(1);
		
		Source<T>::mutex.unlock();

	}

	virtual bool isConnected() const
	{
		return size > 0;
	}

	virtual void disconnect()
	{
		THROWEXCEPTION("Can't disconnect from a splicer\n");
	}
	
	virtual void receive(T packet)
	{
		// this is in an own function to later have the possibility to process
		// the packets in an own thread; for now, I think this is not needed
		process(packet);
	}

private:
	inline void process(T packet)
	{
		if (!Source<T>::sleepUntilConnected()) {
			DPRINTF("Can't wait for connection, perhaps the program is shutting down?");
			return;
		}
		
		/* By processing the array over a mutex proctected variable size
		 * it should be safe to add another destination.
		 * In the worst case, we don't push the packet to a newly added destination.
		 */
		size_t sz = size;
		if (sz > 1) // using this strange construct because size-1 could wrap
			packet->addReference(sz - 1);

		size_t i;
		for (i = 0; i < sz; i++) {
			destinations[i]->receive(packet);
		}
	}

	static const size_t capacity = 10;
	Destination<T>* destinations[capacity];
	size_t size;
};

/** This implements a wrapper on which every other class could gain the ability to
 *  send its flow to _several_ other classes. This couldn't be merged into the class itself
 *  because we would loose the ability to preseve the module over a reconfiguration attempt.
 *  (We need to destroy the splitter on disconnect)
  */  
template <typename T>
struct SplitterAdapter
	: Source<typename T::src_value_type>, Destination<typename T::dst_value_type>, Module
{
	typedef typename T::dst_value_type dst_value_type;
	typedef typename T::src_value_type src_value_type;
	
	SplitterAdapter(T* obj) : adaptee(obj)
	{
		splitter = new ConnectionSplicer<src_value_type>();
		adaptee->connectTo(splitter);
	}

	~SplitterAdapter()
	{
		delete splitter;
	}
	
	virtual void connectTo(Destination<src_value_type>* d)
	{
		splitter->connectTo(d);
	}

	virtual bool isConnected() const
	{
		return adaptee->isConnected();
	}

	virtual void disconnect()
	{
		adaptee->disconnect();
	}
	
	virtual void receive(dst_value_type packet)
	{
		adaptee->receive(packet);
	}
	
	virtual void performStart()
	{
		adaptee->start();
		splitter->start();
	}
	
	virtual void performShutdown()
	{
		adaptee->shutdown();
		splitter->shutdown();
	}

private:
	ConnectionSplicer<src_value_type>* splitter;
	T* adaptee;
};

#endif

