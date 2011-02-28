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

#include "common/msg.h"
#include "core/InstanceManager.h"

/**
 * represents an instance which can be managed by ResourceManager
 * this class does not have much functionality, whole management process
 * should be handled in InstanceManager
 */
template<class T>
class ManagedInstance
{
	friend class InstanceManager<T>;

	private:
		InstanceManager<T>* myInstanceManager;
		volatile int32_t referenceCount;
#if defined(DEBUG)
        bool deletedByManager;
#endif

	public:
		ManagedInstance(InstanceManager<T>* im)
			: myInstanceManager(im), referenceCount(0)

		{
#if defined(DEBUG)
            deletedByManager = (im != 0 ? false : true);
#endif
		}

#if defined(DEBUG)
        virtual ~ManagedInstance()
        {
            // check if this instance was properly deleted by the instance manager
            if (!deletedByManager) {
                THROWEXCEPTION("RefCountInstance was not deleted by InstanceManager!");
            }
        }
#endif

		/**
		 * called by code which wants to use this instance
		 * increases the reference count for this instance and this needs to be decreased
		 * when instance is not used any more
		 */
		inline void addReference(int count = 1)
		{
			myInstanceManager->addReference(static_cast<T*>(this), count);
		}

		/**
		 * called by code which does not need this instance any more
		 */
		inline void removeReference()
		{
			myInstanceManager->removeReference(static_cast<T*>(this));
		}
};

#endif
