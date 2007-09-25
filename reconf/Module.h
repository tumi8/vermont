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
		: exitFlag(false), running(false)
	{ 
	};
	
	virtual ~Module()
	{
		if (running) {
			THROWEXCEPTION("you must shutdown the module first before destroying it!");
		}
	}
	
	/**
	 * is called before module receives input from other modules
	 * e.g. used to start threads
	 */
	void start(bool fail_if_already_running = true)
	{
		if (running && !fail_if_already_running)
			return;

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
	void shutdown(bool fail_if_not_running = true)
	{
		if (!running && !fail_if_not_running)
			return;

		ASSERT(running, "module must be in state running when it is shut down");
		
		notifyShutdown();
		performShutdown();
		running = false;
	}

	
protected:
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
	
	/**
	 * returns current value of exitFlag
	 * (workaround for g++ compiler bug)
	 */
	virtual bool getExitFlag() const
	{
		return exitFlag;
	}

};

#endif

