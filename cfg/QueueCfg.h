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
	: public CfgHelper<ConnectionQueue<T>, QueueCfg<T> >
{
public:
	friend class ConfigManager;
	
	virtual std::string getName() 
	{ 
		// see below why this hack is needed
		T t;
		return get_name(t); 
	}

	virtual QueueCfg<T>* create(XMLElement* e) {
		assert(e != NULL);
		assert(e->getName() == getName());
		return new QueueCfg<T>(e);
	}
	
	virtual ~QueueCfg() { }
	
	ConnectionQueue<T>* createInstance()
	{
		XMLNode* n = this->_elem->getFirstChild("maxSize");
		if (!n) // create a new queue with its default size
			return CfgHelper<ConnectionQueue<T>, QueueCfg<T> >::instance = new ConnectionQueue<T>();

		int maxSize = atoi(n->getFirstText().c_str());
		CfgHelper<ConnectionQueue<T>, QueueCfg<T> >::instance = new ConnectionQueue<T>(maxSize);
		return CfgHelper<ConnectionQueue<T>, QueueCfg<T> >::instance;
	}

	virtual bool deriveFrom(QueueCfg<T>* old)
	{
		// FIXME: queue size and other parameters
		transferInstance(old);
		return true;
	}
	
protected:
	QueueCfg(XMLElement* e)
		: CfgHelper<ConnectionQueue<T>, QueueCfg<T> >(e)
	{
	}
};


class Packet;
typedef QueueCfg<Packet*> PacketQueueCfg;

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
