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

#if !defined(REFCOUNTINSTANCE_H)
#define REFCOUNTINSTANCE_H

template<class T> class InstanceManager;

/**
 * helper class to resolve bidirectional dependency between
 * InstanceManager and ManagedInstance
 */
template<class T>
class RefCountInstance 
{
	friend class InstanceManager<T>;

	protected:
		unsigned int referenceCount;
#if defined(DEBUG)
        bool deletedByManager;
#endif


	public:
		RefCountInstance() : referenceCount(0)
#if defined(DEBUG)
              , deletedByManager(false)
#endif
		{
		}

#if defined(DEBUG)
        virtual ~RefCountInstance()
        {
            // check if this instance was properly deleted by the instance manager
            if (!deletedByManager) {
                THROWEXCEPTION("RefCountInstance was not deleted by InstanceManager!");
            }
        }
#endif

};

#endif
