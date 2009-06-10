#ifndef MODULE_H
#define MODULE_H

/**
	@author Peter Baumann <siprbaum@users.berlios.de>
*/

#include "Source.h"
#include "common/Sensor.h"

using namespace std;

class Module
	: public Sensor
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
	void notifyShutdown(bool shutdownProperly);
	
	/**
	 * shuts down the module
	 * function only returns when module is shut down!
	 */
	void shutdown(bool fail_if_not_running = true, bool shutdownProperly = false);
	
	/**
	 * is called when reconfiguration of vermont is complete
	 * may be overwritten by subclasses
	 */
	virtual void postReconfiguration() { /* override this in the modules you need */ }
	
	/**
	 * called to inform the module that now we are doing a reconfiguration 
	 * Could be used to invalidate some local stuff which will get invalid after
	 * the reconfiguration (e.g. the templates)
	 * is called *after* modules were disconnected
	 * 
	 * May be overwritten by subclasses 
	 */
	virtual void onReconfiguration1() { /* override this in the modules you need */ }
	
	/**
	 * called after ALL modules were informed on the reconfiguration, so
	 * they could to some local cleanup.
	 * is called directly after all modules have processed onReconfiguration1
	 *
	 * May be overwritten by subclasses
	 */
	virtual void onReconfiguration2() { /* override this in the modules you need */ }	
	
	/**
	 * called immediately before module is disconnected from successing modules,
	 * modules preceding the module are already disconnected
	 * ATTENTION: there MAY be another element still be processed in the module,
	 * that means the module programmer MUST pay attention to deadlocks
	 */
	virtual void preReconfiguration() { /* override this in the modules you need */ }

	/**
	 * notifies Vermont to be shut down immediately
	 */
	void shutdownVermont();
	

protected:
	/**
	 * is called when module is started
	 * may be overwritten by subclasses
	 */
	virtual void performStart()	{ }
	
	/**
	 * is called when module is shutdown, function may assume that all preceding modules are already
	 * shut down and do not forward any elements any more
	 * may be overwritten by subclasses
	 */
	virtual void performShutdown() { }
	
	/**
	 * returns current value of exitFlag
	 * (workaround for g++ compiler bug)
	 */
	virtual bool getExitFlag() const;
	
	/**
	 * returns current value of shutdownProperly
	 * (workaround for g++ compiler bug)
	 */
	virtual bool getShutdownProperly() const;
	
	
	bool exitFlag;		/**< notifies module that shutdown is imminent */
	bool running;		/**< true if module is running, false if it is shut down */
	/**
	 * true if module has to shut down properly and to export all data to succeeding modules
	 * ATTENTION: it is assumed that when this variable is set to true, no data will be
	 * input to the module any more
	 */
	bool shutdownProperly; 
	
};

#endif

