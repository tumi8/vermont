/*
 * Vermont Configuration Subsystem
 * Copyright (C) 2009 Vermont Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef QUEUECFG_H_
#define QUEUECFG_H_

/**
	@author Peter Baumann <siprbaum@users.berlios.de>
*/

#include "core/Cfg.h"

#include "core/ConnectionQueue.h"

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
		if (!maxSize) // create a new queue with its default size
			return CfgHelper<ConnectionQueue<T>, QueueCfg<T> >::instance = new ConnectionQueue<T>();

		CfgHelper<ConnectionQueue<T>, QueueCfg<T> >::instance = new ConnectionQueue<T>(maxSize);
		return CfgHelper<ConnectionQueue<T>, QueueCfg<T> >::instance;
	}

	virtual bool deriveFrom(QueueCfg<T>* old)
	{
		if (this->maxSize != old->maxSize)
			return false;

		return true;
	}
	
protected:
	QueueCfg(XMLElement* e)
		: CfgHelper<ConnectionQueue<T>, QueueCfg<T> >(e, "QueueCfg<unspecified>"), maxSize(0)
	{
		// set the correct name in CfgHelper
		this->name = getName();
		
		if (!e)
			return;
		
		maxSize = this->getInt("maxSize", 0);
	}
	
private:
	size_t maxSize;
};


class Packet;
class IpfixRecord;

typedef QueueCfg<Packet*> PacketQueueCfg;
typedef QueueCfg<IpfixRecord*> IpfixQueueCfg;

// this hack with template specialization is needed because 
// gcc (GCC) 4.1.3 20070812 (prerelease) (Debian 4.1.2-15)
// has in ICE if I used a static variable for the name
template<typename X> inline std::string get_name(const X x)
{
	return "";
}

template<> inline std::string get_name<Packet*>(Packet* x)
{
	return "packetQueue";
}

template<> inline std::string get_name<IpfixRecord*>(IpfixRecord* x)
{
	return "ipfixQueue";
}

#endif /*QUEUECFG_H_*/
