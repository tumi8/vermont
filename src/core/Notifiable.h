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

#ifndef NOTIFIABLE_H_
#define NOTIFIABLE_H_

// forward declaration
class Timer;

/**
 * to be implemented by modules which need to be triggered after a timeout
 * trigger is always a module which implements interface Timer
 */
class Notifiable
{
public:
	Notifiable() : timer(0) {}
	virtual ~Notifiable() {}

	/**
	 * function is called after the specified timeout in function
	 * Timer::addTimeout
	 * @param flag flag which was passed to Timer::addTimeout
	 */
	virtual void onTimeout(void* dataPtr) = 0;

	inline void useTimer(Timer* t) { timer = t; }

protected:
	Timer* timer;
};

#endif /*NOTIFIABLE_H_*/
