#ifndef QUEUECFG_H_
#define QUEUECFG_H_

/**
	@author Peter Baumann <siprbaum@users.berlios.de>
*/

#include "Cfg.h"

#include "reconf/ConnectionQueue.h"

#include <cassert>

template <class T>
class QueueCfg
	: public Cfg
{
public:
	friend class ConfigManager;
	
	virtual std::string getName() 
	{ 
		// see below why this hack is needed
		T* t;
		return get_name(t); 
	}

	virtual QueueCfg<T>* create(XMLElement* e) {
		assert(e != NULL);
		assert(e->getName() == getName());
		return new QueueCfg<T>(e);
	}
	
	virtual ~QueueCfg() { }
	
	ConnectionQueue<T>* getInstance()
	{
		if (queue)
			return queue;
	
		XMLNode* n = _elem->getFirstChild("maxSize");
		if (!n) // create a new queue with its default size
			return queue = new ConnectionQueue<T>();

		int maxSize = atoi(n->getFirstText().c_str());
		queue = new ConnectionQueue<T>(maxSize);
		return queue;
	}
	
protected:
	QueueCfg(XMLElement* e)
		: Cfg(e), queue(NULL)
	{
	}
	
private:
	ConnectionQueue<T>* queue;
};


class Packet;
typedef QueueCfg<Packet> PacketQueueCfg;

// this hack with template specialization is needed because 
// gcc (GCC) 4.1.3 20070812 (prerelease) (Debian 4.1.2-15)
// has in ICE if I used a static variable for the name
template<typename X> std::string get_name(const X x)
{
	return "";
}

template<> std::string get_name<Packet*>(Packet* x)
{
	return "packetQueue";
}

#endif /*QUEUECFG_H_*/
