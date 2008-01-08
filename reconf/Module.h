#ifndef MODULE_H
#define MODULE_H

/**
	@author Peter Baumann <siprbaum@users.berlios.de>
*/

#include "Source.h"
#include "common/ThreadCPUInterface.h"

#include <list>
#include <sys/types.h>

using namespace std;

class Module 
{

public:	

	
	Module();	
	virtual ~Module();
	
	/**
	 * is called before module receives input from other modules
	 * e.g. used to start threads
	 */
	void start(bool fail_if_already_running = true);
		
	/**
	 * notifies the module about immediate shutdown
	 */
	void notifyShutdown();
	
	/**
	 * shuts down the module
	 * function only returns when module is shut down properly!
	 */
	void shutdown(bool fail_if_not_running = true);
	
	/* this is called before connecting a module */
	/**
	 * is called when reconfiguration of vermont is complete
	 * may be overwritten by subclasses
	 */
	virtual void postReconfiguration() { /* override this in the modules you need */ }
	
	/* called to inform the module that now we are doing a reconfiguration 
	 * Could be used to invalidate some local stuff which will get invalid after
	 * the reconfiguration (e.g. the templates)
	 * 
	 * May be overwritten by subclasses 
	 */
	virtual void preReconfiguration1() { /* override this in the modules you need */ }
	
	/* called after ALL modules were informed on the reconfiguration, so
	 * they could to some local cleanup.
	 *
	 * May be overwritten by subclasses
	 */
	virtual void preReconfiguration2() { /* override this in the modules you need */ }
	
	/**
	 * returns memory usage of this module
	 * ATTENTION: module needs to count memory usage on its own, see variable usedBytes!
	 */
	uint32_t getCurrentMemUsage();
	
	/**
	 * returns number of jiffies used by all threads since last call of this function
	 * @param empty list which will be filled with data
	 */
	void getJiffiesUsed(list<ThreadCPUInterface::JiffyTime>& usedJiffies);
	
	/**
	 * registers given thread id as thread belonging to this module
	 * module code should do this once for each thread that is used by it
	 * @param thread thread id to be registered
	 */
	void registerThreadID(pid_t tid);
	
	/**
	 * registers current thread as thread belonging to this module
	 * module code should do this once for each thread that is used by it
	 */
	void registerCurrentThread();
	
	/**
	 * unregisters given thread id as thread belonging to this module
	 * @param thread thread id to be unregistered
	 */
	void unregisterThreadID(pid_t tid);
	
	/**
	 * unregisters current thread as thread belonging to this module
	 */
	void unregisterCurrentThread();

protected:
	/**
	 * is called when module is started
	 * may be overwritten by subclasses
	 */
	virtual void performStart()	{ }
	
	/**
	 * is called when module is shutdown
	 * may be overwritten by subclasses
	 */
	virtual void performShutdown() { }
	
	/**
	 * returns current value of exitFlag
	 * (workaround for g++ compiler bug)
	 */
	virtual bool getExitFlag() const;
	
	/**
	 * modules must store the amount of used memory in this variable
	 * ATTENTION: this variable is not safely accessed in multi-threading context!
	 * do not store temporary data there, only final values!
	 */
	uint32_t usedBytes;
	
	bool exitFlag;		/**< notifies module that shutdown is imminent */
	bool running;		/**< true if module is running, false if it is shut down */
	
private:
	list<ThreadCPUInterface::JiffyTime> watchedThreads; /** all threads that are used by module */
	Mutex wThreadsMutex; /** mutex for locking watchedThreads */

};

#endif

