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
		CfgHelper<ConnectionQueue<T>, QueueCfg<T> >::instance = new ConnectionQueue<T>(maxSize, multiplePredecessors);
		return CfgHelper<ConnectionQueue<T>, QueueCfg<T> >::instance;
	}

	/** this method can be called for 2 reasons:
	 * 	1. if the instance needs a timer and in the configuration
	 *  there was no timer in front of the instance
	 *  2. if the instance has more predecessors. In this case multi
	 *  should be set true
	 *
	 *  @multi set true if the called module has more predecessors
	 * 	@return Instance of the ConnectionQueue
	 */
	ConnectionQueue<T>* getQueueInstance(bool multi = false){
		multiplePredecessors = multi;
		return createInstance();
	}

	virtual bool deriveFrom(QueueCfg<T>* old)
	{
		if (this->maxSize != old->maxSize)
			return false;

		return true;
	}
	
protected:
	QueueCfg(XMLElement* e)
		: CfgHelper<ConnectionQueue<T>, QueueCfg<T> >(e, "QueueCfg<unspecified>")
	{
		// set the correct name in CfgHelper
		this->name = getName();
		
		if (!e)
			return;
		
		maxSize = this->getInt("maxSize", 100);
		multiplePredecessors = false;
	}
	
private:
	size_t maxSize;
	bool multiplePredecessors;
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
