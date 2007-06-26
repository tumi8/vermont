/*
 * ManagedInstance
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

#if !defined(MANAGEDINSTANCE_H)
#define MANAGEDINSTANCE_H

#include "msg.h"
#include "InstanceManager.h"
#include "RefCountInstance.h"

/**
 * represents an instance which can be managed by ResourceManager
 * this class does not have much functionality, whole management process
 * should be handled in InstanceManager
 */
template<class T>
class ManagedInstance : public RefCountInstance<T>
{
	friend class InstanceManager<T>;

	private:
		InstanceManager<T>& myInstanceManager;

	public:
		ManagedInstance(InstanceManager<T>& im)
			: myInstanceManager(im)
		{
		}

		/**
		 * called by code which wants to use this instance
		 * increases the reference count for this instance and this needs to be decreased
		 * when instance is not used any more
		 */
		inline void addReference(int count = 1)
		{
			myInstanceManager.addReference(this, count);
		}

		/**
		 * called by code which does not need this instance any more
		 */
		inline void removeReference()
		{
			myInstanceManager.removeReference(this);
		}
};

#endif
