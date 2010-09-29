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
		return "";
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


template <> std::string QueueCfg<Packet*>::getName()
{
	return "packetQueue";
}

template <> std::string QueueCfg<IpfixRecord*>::getName()
{
	return "ipfixQueue";
}

#endif /*QUEUECFG_H_*/
