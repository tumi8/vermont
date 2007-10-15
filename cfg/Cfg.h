#ifndef CFG_H_
#define CFG_H_

#include "cfg/XMLElement.h"
#include "reconf/Module.h"

#include "reconf/ConnectionQueue.h"
#include "reconf/ConnectionSplicer.h"
#include "reconf/Timer.h"

#include <exception>
#include <string>

/**
 * This excecption is thrown if someone try's to get an element which
 * doesn't exist
 */
class IllegalEntry 
	: public std::exception
{
		
};

enum timeUnit {
	SEC = 1,
	mSEC= 1000,
	uSEC= 1000000,
};

// some constant values (bitmaks
#define NO_ADAPTER	0
#define NEED_SPLITTER	1 << 0
#define NEED_TIMEOUT	1 << 1


class CfgBase
{
public:
	CfgBase(XMLElement* e): _elem(e) { }
	virtual ~CfgBase() { }
	
	/** returns the name (as written in the XML file) */
	virtual std::string getName() = 0;

	/** return a string value of an elemen 
	 * @param name the name of the element
	 *  @param elem the XMLElement we want to start the search, default is the root of the node
	 */
	std::string get(const std::string& name, XMLElement* elem = NULL) throw(IllegalEntry);

	/** searches for a optinal config entry, returns the emtpy string of not found
	 *  @param name the name of the element
	 *  @param elem the XMLElement we want to start the search, default is the root of the node
	 */  
	std::string getOptional(const std::string&name, XMLElement* elem = NULL);
	
	/** returns the integer value of an XML config entry
	 * @param name the name of the element
	 * If there is no such element in the XML file, it returns def
	 */
	int getInt(const std::string& name, int def = 0, XMLElement* elem = NULL);
	
	/**
	 * returns the time value specified in the element in a given unit
	 * @param name the name of the element
	 * @param unit the unit in which the time should be returned
	 * @param elem the element whose children are search for the given name
	 */
	unsigned int getTimeInUnit(const std::string& name, timeUnit unit,
				   unsigned def = 0, XMLElement* elem = NULL);
	

	XMLElement* _elem;
};

class Cfg : public CfgBase
{
public:
	friend class ConfigFile;

	
	virtual Cfg* create(XMLElement* e) = 0;//{ return NULL; };
	virtual ~Cfg() { }

	/** returns the name (as written in the XML file) */
	virtual std::string getName() = 0;

	/** ID given in the XML file */
	unsigned int getID();

	/** returns all the ID's of the next processing elements in the pipeline */
	std::vector<unsigned int> getNext();

	/** returns true if we could reuse the other instance 
	 *  @param other Cfg describing the old/other elemen
	 */
	virtual bool deriveFrom(CfgBase* other) = 0;

	/** connectes this module with the module from \other
	 *  @param other the other Cfg
	 */
	virtual void connectInstances(Cfg* other) = 0;
	virtual void disconnectInstances() = 0;

	/* start/stop the instance */
	virtual void start(bool fail_if_already_running = true) = 0;
	virtual void shutdown(bool fail_if_not_running = true) = 0;

	/** this method will delete the instance */
	virtual void freeInstance() = 0;


	// THE METHODS BELOW SHOULD NEVER BE CALLED DIRECTLY; THE SHOULD BE PRIVATE, BUT WE
	// NEED A GENERIC BASE CLASS IN CfgHelper, AND MAKING THEM PROTECTED DOESN'T ALLOW THEM TO BE
	// CALLED IN CfgHelper, SO THEY ARE PUBLIC. YES, WE COULD CREATE AN INTERMEDIATE CLASS WHICH EXPOSES
	// THESE FUNCTIONS, BUT I DON'T THINK THIS IS WORTH THE COMPLEXITY
	
	/** returns an instance of the module which the config element describes */
	virtual Module* getInstance() = 0;
	
	/** returns a Queue, which is used as a TimoutAdapter */
	virtual Module* getQueueInstance() = 0;

protected:	

	Cfg(XMLElement* e) : CfgBase(e) { }
};


/**
 * This is a helper class to avoid code duplication over all the other Cfg subclasses
 * It helps to manage the adapter classes like Splitter, which are otherwise very difficult
 * to represent.
 *
 * The idea is that we create the following module hierarchy if neccessary
 * 
 *	-----------------------------------------
 *	|	QUEUE  ->  MODULE  -> SPLITTER	|
 * 	-----------------------------------------
 *
 * From an outside view this box with its 3 modules looks like one module. 
 */
template <typename InstanceType, typename ConfigType>
class CfgHelper
	: public Cfg 
{
public:
	
	CfgHelper(XMLElement* elem) 
		: Cfg(elem), instance(NULL), splitter(NULL), queue(NULL), notifiable(NULL) 
	{ 
	}
	
	virtual ~CfgHelper()
	{ 
		shutdown(false);
		freeTimeoutAdapter();
		freeInstance();
		freeSplitter();
	}
	
	/** starts the module */
	virtual void start(bool fail_if_already_running = true)
	{
		if (splitter)
			splitter->start(fail_if_already_running);
		if (instance)
			instance->start(fail_if_already_running);
		if (queue)
			queue->start(fail_if_already_running);
	}

	/* stops the module */
	virtual void shutdown(bool fail_if_not_running = true)
	{
		if (queue)
			queue->shutdown(fail_if_not_running);
		if (instance)
			instance->shutdown(fail_if_not_running);
		if (splitter)
			splitter->shutdown(fail_if_not_running);
	}

	/** returns the module instance (if neccessary, it will create it */
	InstanceType* getInstance()
	{
		if (instance != NULL)
			return instance;
		
		return createInstance();
	}
	
	/** this method gets called *ONLY* if the instance needs a timer and the
	 *  in the configuration there was no timer in front of the instance
	 */
	ConnectionQueue<typename InstanceType::dst_value_type>* getQueueInstance()
	{
		if (!queue) {
			DPRINTF("Automatic creation of Queue (we need a Timer in front of us");
			queue = new ConnectionQueue<typename InstanceType::dst_value_type>(1);
		}
		
		queue->connectTo(getInstance());
		Notifiable* n = dynamic_cast<Notifiable*>(instance);
		if (n)
			n->useTimer(queue);
		return queue;
	}
	
	/** returns true if we could reuse the other instance 
	 *  @param other Cfg describing the old/other element
	 */
	virtual bool deriveFrom(CfgBase* old)
	{
		ConfigType* cfg = dynamic_cast<ConfigType*>(old);
		if (cfg)
			return this->deriveFrom(cfg);

		THROWEXCEPTION("Can't derive %s from %s", 
				this->getName().c_str(), old->getName().c_str());
		return false;
	}

	/** every derived class has to implement its specalised deriveFrom method */
	virtual bool deriveFrom(ConfigType* old) = 0;
	
	/** Every derived class must provide a method to actually create a instance */
	virtual InstanceType* createInstance() = 0;

	void freeInstance()
	{
		if (!instance)
			return;
		
		delete instance;
		instance = NULL;
	}
	
	/** connects the instances hold in the to configurations
	 *  This method takes care to also create the neccessary adapters/wrappers
	 */ 
	virtual void connectInstances(Cfg* other)
	{
		// create an instance if neccessary
		instance = getInstance();

		// the other side has to be of type Destination<I::src_value_type>
		Destination<typename InstanceType::src_value_type >* dest = NULL;

		// check if we need a ConnectionQueue as TimeoutAdapter
		notifiable = dynamic_cast<Notifiable*>(other->getInstance());
		Timer* timer = dynamic_cast<Timer*>(instance);
		
		if (notifiable) { // the other one is a notifiable and needs a timer set
			if (timer == NULL) { // we are not a timer
				ConnectionQueue<typename InstanceType::src_value_type >* destQueue;
				destQueue = dynamic_cast<ConnectionQueue< typename InstanceType::src_value_type>* >
						(other->getQueueInstance());

				if (!destQueue)
					THROWEXCEPTION("Queue is not a timer; this should never happen");
				
				timer = destQueue;
				dest  = destQueue;
			}
			notifiable->useTimer(timer);
		}

		if (!dest) { // dest wasn't set yet
			dest = dynamic_cast<Destination< typename InstanceType::src_value_type>* >
					(other->getInstance());
			if (!dest)
				THROWEXCEPTION("Unexpected error: can't cast %s to matching Destination<>",
						other->getName().c_str());
		}
		
		// check if we need a splitter
		if (this->getNext().size() > 1) {
			printf("-----SPLITTER\n");
			if (!splitter) {
				splitter = new ConnectionSplicer<typename InstanceType::src_value_type>();
				instance->connectTo(splitter);
			}
			splitter->connectTo(dest);
		} else
			instance->connectTo(dest);
	}

	/** disconnect the instances and deletes an automaticly created splitter */
	void disconnectInstances()
	{
		// reset the timer to NULL so that the object could be deleted
		Timer* timer = dynamic_cast<Timer*>(instance);
		if (timer && notifiable) {
			notifiable->useTimer(NULL);
		}
		
		/* dont disconnect/delete the queue, we could be still connected to a source
		if (queue) {
			queue->disconnect();
			freeTimeoutAdapter();
		}
		*/

		// this will disconnect either the splitter or the other module
		if (instance)
			instance->disconnect();
		
		freeSplitter();
	} 

protected:
	
	/** helper method to transfer one instance to another Cfg 
	 *  @param other the Cfg entry we want to transfer the instance to
	 */
	inline void transferInstance(CfgHelper<InstanceType, ConfigType>* other)
	{
		if (other->queue) {
			other->queue->disconnect();
			queue = other->queue; // get the queue from the other element
			other->queue = NULL;  // and remove it from the other side
		}
		
		instance = other->instance;
		other->instance = NULL;
	}

	InstanceType* instance;
	
	ConnectionSplicer<typename InstanceType::src_value_type>* splitter;
	ConnectionQueue<typename InstanceType::dst_value_type>* queue;
	
	Notifiable* notifiable;
private:
	// this methods are helper functions to clean our mess up and reset the values to sane default
	inline void freeTimeoutAdapter()
	{
		if (!queue)
			return;
		delete queue;
		queue = NULL;
	}

	/** delete the splitter if any */
	inline void freeSplitter()
	{
		if (!splitter)
			return;
		delete splitter;
		splitter = NULL;
	}
};

#endif /*CFG_H_*/
