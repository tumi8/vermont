/*
 * Vermont 
 * Copyright (C) 2007 Tobias Limmer <http://www7.informatik.uni-erlangen.de/~limmer>
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

#ifndef ADAPTER_H_
#define ADAPTER_H_


#include "Source.h"
#include "Destination.h"
#include "Module.h"

/**
 * this class just sends incoming packets to the next module
 * without doing anything
 * this class is the baseclass for modules which do not interpret elements
 * but control the forwarding process of them
 */
template <class T>
class Adapter
	: public Source<T>, public Destination<T>, public Module
{
public:
	typedef typename Source<T>::src_value_type src_value_type;
	typedef typename Destination<T>::dst_value_type dst_value_type;
	
	Adapter()
	{
	}
	
	virtual ~Adapter()
	{
		shutdown(false);
	}
	
	// inherited from Destination<T>
	virtual void receive(T element)
	{
		Source<T>::send(element);
	}

	virtual void notifyQueueRunning() {
		Source<T>::sendQueueRunningNotification();
	}
};

#endif /*ADAPTER_H_*/
