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
	 * called by ConfigManager to start the module and to restart the module after reconfiguration
	 * performStart() is called if the module is not running
	 * postReconfiguration() is called if module is running and if fail_if_already_running==false
	 * @param fail_if_already_running if true, throws exception if module is already running 
	 */
	void start(bool fail_if_already_running = true);
	
	/**
	 * called by ConfigManager or preceding module is shutdown the module
	 * notifyShutdown(shutdownProperly) and performShutdown() are called if module is running 
	 * function only returns when module is shut down!
	 * @param fail_if_already_running if true, throws exception if module is not running 
	 */
	void shutdown(bool fail_if_not_running = true, bool shutdownProperly = false);
		
	/**
	 * called during reconfiguration immediately before module is disconnected from successive modules,
	 * modules preceding the module are already disconnected
	 * NOTE: This method is called on all modules in the graph, independently of
	 * whether the parameterization of the module will remain unchanged in the new
	 * configuration.
	 * ATTENTION: there MAY be another element still be processed in the module,
	 * that means the module programmer MUST pay attention to deadlocks
	 */
	virtual void preReconfiguration() { /* override this in the modules you need */ }

	/**
	 * called during reconfiguration after module is disconnected from successive modules and 
	 * before calling onReconfiguration2()
	 * Could be used to invalidate some local stuff which will get invalid after
	 * the reconfiguration (e.g. the templates).
	 * NOTE: This method is called on all modules in the graph, independently of
	 * whether the parameterization of the module will remain unchanged in the new
	 * configuration.
	 * is called *after* modules were disconnected
	 * 
	 * May be overwritten by subclasses 
	 */
	virtual void onReconfiguration1() { /* override this in the modules you need */ }
	
	/**
	 * called after onReconfiguration1() was called on all modules
	 * NOTE: This method is called on all modules in the graph, independently of
	 * whether the parameterization of the module will remain unchanged in the new
	 * configuration.
	 *
	 * TODO: The two-step approach of onReconfiguration1 and onReconfiguration2 was used 
	 * to destroy Templates via destroyed flag in TemplateInfo. Since this flag was removed,
	 * we could probably do with a single onReconfiguration() method instead. 
	 *
	 * May be overwritten by subclasses
	 */
	virtual void onReconfiguration2() { /* override this in the modules you need */ }	
	
	/**
	 * notifies Vermont to be shut down immediately
	 */
	void shutdownVermont();
	

protected:
	/**
	 * called in shutdown() before just before performShutdown() 
	 * sets exitFlag=true and the shutdownProperly flag to inform threads in module about shutdown
	 */
	void notifyShutdown(bool shutdownProperly);
	
	/**
	 * is called when the module is started and before module receives input from other modules
	 * can be used to start threads etc.
	 *
	 * May be overwritten by subclasses
	 */
	virtual void performStart()	{ }
	
	/**
	 * is called after the reconfiguration of the module is complete under the condition
	 * that the module already existed in the preceding configuration.
	 * As the module already existed in the preceding configuration, it may
	 * now do some reinitialization work, such as resending templates.
	 * may be overwritten by subclasses
	 */
	virtual void postReconfiguration() { /* override this in the modules you need */ }
	
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

