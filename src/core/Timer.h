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


#ifndef TIMER_H_
#define TIMER_H_

#include "Notifiable.h"

/**
 * pure virtual interface for modules which implement Timer-functionality
 * to call Notifiables which need to be triggered after some time
 */
class Timer
{
public:
	virtual ~Timer() {}
	
	/**
	 * adds a timeout for the given Notifiable. Notifiable::onTimeout is called 
	 * after the specified timeout
	 * @param n Notifiable to be called
	 * @param ts absolute timeout when call is due
	 * @param flag user-defined flag which is passed to Notifiable::onTimeout, so that
	 *             Notifiable may distinguish between different timeouts
	 */
	virtual void addTimeout(Notifiable* n, struct timespec& ts, void* dataPtr = 0) = 0;
};

#endif /*TIMER_H_*/
