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
	: public Cfg, public CfgHelper< ConnectionQueue<T> >
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
	
	ConnectionQueue<T>* getInstance()
	{
		if (CfgHelper<ConnectionQueue<T> >::instance)
			return CfgHelper<ConnectionQueue<T> >::instance;
	
		XMLNode* n = _elem->getFirstChild("maxSize");
		if (!n) // create a new queue with its default size
			return CfgHelper<ConnectionQueue<T> >::instance = new ConnectionQueue<T>();

		int maxSize = atoi(n->getFirstText().c_str());
		CfgHelper<ConnectionQueue<T> >::instance = new ConnectionQueue<T>(maxSize);
		return CfgHelper<ConnectionQueue<T> >::instance;
	}

	virtual bool deriveFrom(Cfg* o)
	{
		QueueCfg<T>* cfg = dynamic_cast< QueueCfg<T>* >(o);
		if (cfg)
			return deriveFrom(cfg);

		THROWEXCEPTION("Derive is only allowed from within the same type");
		return false;
	}

	virtual bool deriveFrom(QueueCfg<T>* old)
	{
		// FIXME: queue size and other parameters
		transferInstance(old);
		return true;
	}
	
	virtual void connectInstances(Cfg* other)
	{
		CfgHelper<ConnectionQueue<T> >::instance = getInstance();

		int need_adapter = 0;
		need_adapter |= ((getNext().size() > 1) ? NEED_SPLITTER : NO_ADAPTER);

		if ((dynamic_cast<Notifiable*>(other->getInstance()) != NULL) &&
		    (dynamic_cast<Timer*>(CfgHelper<ConnectionQueue<T> >::instance) == NULL))
			need_adapter |= NEED_TIMEOUT;
		
		CfgHelper<ConnectionQueue<T> >::connectTo(other->getInstance(), need_adapter);
	}
	
protected:
	QueueCfg(XMLElement* e)
		: Cfg(e)
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
