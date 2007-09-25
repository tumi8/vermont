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

#ifndef TIMEOUTADAPTER_H_
#define TIMEOUTADAPTER_H_
	
#include "ThreadSafeAdapter.h"

/**
 * uses an internal thread to pass given elements to next module
 * asynchronously. Queue contains a buffer with variable length
 * for passing data from calling threads to own thread
 */
template <class T>
class TimeoutAdapter : public ThreadSafeAdapter
{
public:
	TimeoutAdapter()
	{
	}
	
	virtual ~TimeoutAdapter()
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
	TimeoutSemaphore
	Thread thread;
};

#endif /*TIMEOUTADAPTER_H_*/
