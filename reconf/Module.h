#ifndef MODULE_H
#define MODULE_H

/**
	@author Peter Baumann <siprbaum@users.berlios.de>
*/

#include "Source.h"

class Module : public virtual BaseSource, public virtual BaseDestination 
{
protected:
	bool exitFlag;		/**< notifies module that shutdown is imminent */
	bool running;		/**< true if module is running, false if it is shut down */

public:	
	Module() 
		: exitFlag(true), running(false)
	{ };
	
	/**
	 * is called before module receives input from other modules
	 * e.g. used to start threads
	 */
	void start() 
	{
		ASSERT(!running, "module must not be in state 'running' when started");
		running = true;
		exitFlag = false;
		performStart();
	}
	
	/**
	 * notifies the module about immediate shutdown
	 */
	void notifyShutdown()
	{
		exitFlag = true;
	}
	
	/**
	 * shuts down the module
	 * function only returns when module is shut down properly!
	 */
	void shutdown() 
	{
		ASSERT(running, "module must be in state running when it is shut down");
		
		notifyShutdown();
		performShutdown();
		running = false;
	}

	/**
	 * is called when module is started
	 * may be overwritten by subclasses
	 */
	virtual void performStart()
	{
	}
	
	/**
	 * is called when module is shutdown
	 * may be overwritten by subclasses
	 */
	virtual void performShutdown()
	{
	}

};

#endif

