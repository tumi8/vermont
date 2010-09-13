/*
 * Instance Manager
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

#if !defined(INSTANCEMANAGER_H)
#define INSTANCEMANAGER_H

#include "common/Mutex.h"
#include "SensorManager.h"
#include "common/Sensor.h"
#include "common/defs.h"
#include "common/LockfreeMultiQueue.h"

#include <queue>
#include <list>
#include <algorithm>

using namespace std;

/**
 * manages instances of the given type to avoid news/deletes in program
 * managed types *should* be inherited from ManagedInstance
 * ATTENTION: this class internally handles *pointers* of the given type
 */
template<class T>
class InstanceManager : public Sensor
{
	private:
#if defined(DEBUG)
		list<T*> usedInstances;	// instances with active references (only used for debugging purposes)
#endif
		LockfreeMultiQueue<T*>* freeInstances;// unused instances
		Mutex mutex;			// we wanna be thread-safe
		static const int DEFAULT_NO_INSTANCES = 1000;
		uint32_t statCreatedInstances; /**< number of created instances, used for statistical purposes */

	public:
		InstanceManager(string type, int preAllocInstances = DEFAULT_NO_INSTANCES)
			: statCreatedInstances(0)
		{
			//printf("##instanceManager\n");
			int qSize = (preAllocInstances > DEFAULT_NO_INSTANCES) ? preAllocInstances : DEFAULT_NO_INSTANCES;
			freeInstances = new LockfreeMultiQueue<T*>(qSize);
			for (int i=0; i<preAllocInstances; i++) {
				freeInstances->push(new T(this));
			}
			statCreatedInstances = preAllocInstances;
			//**TODO size of the LockfreeMultiQueue have to be added to usedBytes*/
			usedBytes += sizeof(InstanceManager<T>)+preAllocInstances*(sizeof(T)+4);
			SensorManager::getInstance().addSensor(this, "InstanceManager (" + type + ")", 0);
		}

		virtual ~InstanceManager()
		{
            SensorManager::getInstance().removeSensor(this);
#if defined(DEBUG)
			if (!usedInstances.empty()) {
				DPRINTF("freeing instance manager, although there are still %d used instances", usedInstances.size());
			}
#endif
			T *obj;
			while (freeInstances->pop(&obj)) {
#if defined(DEBUG)
				obj->deletedByManager = true;
#endif
				delete obj;
			}
			delete freeInstances;
		}

		/**
		 * returns an unused instance of the managed objects
		 * if no unused instance is available, a new one will be created
		 */
		inline T* getNewInstance()
		{
			T* instance;
#if !defined(IM_DISABLE)

			if (!freeInstances->pop(&instance)) {
				// create new instance
				__sync_add_and_fetch(&statCreatedInstances,1);
				instance = new T(this);
				__sync_add_and_fetch(&usedBytes,sizeof(T)+4);
			}

#if defined(DEBUG)
			mutex.lock();
			DPRINTF("adding used instance 0x%08X", (void*)instance);
			usedInstances.push_back(instance);
			mutex.unlock();
#endif

			instance->referenceCount++;

#else // IM_DISABLE
			instance = new T(this);
			instance->referenceCount++;
#endif // IM_DISABLE

			return instance;
		}

		inline void addReference(T* instance, int count)
		{
#if defined(DEBUG)
			mutex.lock();
#endif
			__sync_add_and_fetch(&(instance->referenceCount),count);
#if defined(DEBUG)
#if !defined(IM_DISABLE)
			// the referenceCount MUST NEVER be zero and still be used by some code
			if (instance->referenceCount-count == 0) {
				THROWEXCEPTION("instance reference counter was zero and was still used");
			}
			// this instance should be in the used list, else there is something wrong
			if (find(usedInstances.begin(), usedInstances.end(), instance) == usedInstances.end()) {
				THROWEXCEPTION("instance (0x%08X) is not managed by InstanceManager", (void*)instance);
			}
#endif // IM_DISABLE
			mutex.unlock();
#endif // DEBUG
		}

		inline void removeReference(T* instance)
		{
			__sync_add_and_fetch(&(instance->referenceCount),-1);

			if (instance->referenceCount == 0) {
#if !defined(IM_DISABLE)

				if(!freeInstances->push(instance)){
					delete instance;
					__sync_add_and_fetch(&usedBytes,-(sizeof(T)+4));
				}
#if defined(DEBUG)
				mutex.lock();
				typename list<T*>::iterator iter = find(usedInstances.begin(), usedInstances.end(), instance);
				if (iter == usedInstances.end()) {
					THROWEXCEPTION("instance (0x%08X) is not managed by InstanceManager", (void*)instance);
				}
				DPRINTF("removing used instance 0x%08X", (void*)instance);
				usedInstances.erase(iter);
				mutex.unlock();
#endif
#else // IM_DISABLE
				DPRINTF("removing used instance 0x%08X", (void*)instance);
				instance->deletedByManager = true;
				delete instance;
#endif // IM_DISABLE
			}
#if defined(DEBUG) && !defined(IM_DISABLE)
			if (instance->referenceCount < 0) {
				THROWEXCEPTION("referenceCount of instance is < 0");
			}
#endif
		}

		string getStatisticsXML(double interval)
		{
			char text[200];
			snprintf(text, ARRAY_SIZE(text), "<createdInstances>%u</createdInstances>", statCreatedInstances);
			return string(text);
		}
};


#endif
