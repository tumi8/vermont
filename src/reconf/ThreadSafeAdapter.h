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

#ifndef THREADSAFEADAPTER_H_
#define THREADSAFEADAPTER_H_

#include "Adapter.h"

#include "common/Mutex.h"


/**
 * extension of Adapter module:
 * this adapter cares for thread safe forwarding of elements, but still
 * puts calls through synchronously
 */
template <class T>
class ThreadSafeAdapter : public Adapter<T>
{
public:
	ThreadSafeAdapter()
	{
	}
	
	virtual ~ThreadSafeAdapter()
	{
	}
	
	// inherited from Destination<T>
	virtual void receive(T element)
	{
		mutex.lock();
		send(element);
		mutex.unlock();
	}
	
protected:
	Mutex mutex;
};

#endif /*THREADSAFEADAPTER_H_*/
