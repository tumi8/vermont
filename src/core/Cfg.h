#ifndef CFG_H_
#define CFG_H_

#include "core/XMLElement.h"
#include "core/Module.h"
#include "core/ConnectionQueue.h"
#include "core/ConnectionSplitter.h"
#include "core//Timer.h"

#include <exception>
#include <string>
#include <typeinfo>

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

	/** return a string value of an elemen
	 * @param name the name of the element
	 *  @param elem the XMLElement we want to start the search, default is the root of the node
	 */
	std::string get(const std::string& name, XMLElement* elem = NULL);

	/** searches for an optinal config entry, returns the emtpy string if not found
	 *  @param name the name of the element
	 *  @param elem the XMLElement we want to start the search, default is the root of the node
	 */
	std::string getOptional(const std::string&name, XMLElement* elem = NULL);

	/** returns the integer value of an XML config entry
	 * @param name the name of the element
	 * If there is no such element in the XML file, it returns def
	 */
	int getInt(const std::string& name, int def = 0, XMLElement* elem = NULL);

	/** returns the unsigned integer value with 32bit of an XML config entry
	 * @param name the name of the element
	 * If there is no such element in the XML file, it returns def
	 */
	uint32_t getUInt32(const std::string& name, uint32_t def = 0, XMLElement* elem = NULL);

	/** returns the 64-bit integer value of an XML config entry
	 * @param name the name of the element
	 * If there is no such element in the XML file, it returns def
	 */
	int64_t getInt64(const std::string& name, int64_t def = 0, XMLElement* elem = NULL);

	/** returns the bool  value of an XML config entry
	 * @param name the name of the element
	 * If there is no such element in the XML file, it returns def
	 */
	bool getBool(const std::string& name, bool def = false, XMLElement* elem = NULL);

	/** returns the double value of an XML config entry
	 * @param name the name of the element
	 * If there is no such element in the XML file, it returns def
	 */
	double getDouble(const std::string& name, double def = 0, XMLElement* elem = NULL);

	/**
	 * returns the time value specified in the element in a given unit
	 * @param name the name of the element
	 * @param unit the unit in which the time should be returned
	 * @param elem the element whose children are search for the given name
	 */
	unsigned int getTimeInUnit(const std::string& name, timeUnit unit, uint32_t def = 0,
				   XMLElement* elem = NULL);


	XMLElement* _elem;

private:
	std::string _get(const std::string& name, XMLElement* elem) throw(IllegalEntry);
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
	virtual bool deriveFrom(Cfg* other) = 0;

	virtual void transferInstance(Cfg* other) = 0;

	/** connects this module with the module from \other
	 *  @param other the other Cfg
	 */
	virtual void connectInstances(Cfg* other) = 0;

	virtual void setupWithoutSuccessors() = 0;
	virtual void disconnectInstances() = 0;

	/* start/stop the instance */
	virtual void start(bool fail_if_already_running = true) = 0;
	virtual void shutdown(bool fail_if_not_running = true, bool finishProperly = false) = 0;

	/* see in Module for the documentation for these functions */
	//virtual void postReconfiguration() = 0;
	virtual void onReconfiguration1() = 0;
	virtual void onReconfiguration2() = 0;

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

	CfgHelper(XMLElement* elem, std::string name, bool freeInstance = true)
		: Cfg(elem),
		instance(NULL),
		splitter(NULL),
		queue(NULL),
		notifiable(NULL),
		name(name),
		instanceDeleteAllowed(freeInstance)
	{
	}

	virtual ~CfgHelper()
	{
		DPRINTF("~CfgHelper [%s]\n", this->getName().c_str());
		shutdown(false);
		freeTimeoutAdapter();
		freeInstance();
		freeSplitter();
	}

	virtual std::string getName()
	{
		return name;
	}

	/** starts the module */
	virtual void start(bool fail_if_already_running = true)
	{
		// create instance, if not already present
		if ((splitter==NULL) && (instance==NULL) && (queue==NULL))
			createInstance();

		if (splitter)
			splitter->start(fail_if_already_running);
		if (instance)
			instance->start(fail_if_already_running);
		if (queue)
			queue->start(fail_if_already_running);
	}

	/* stops the module */
	virtual void shutdown(bool fail_if_not_running = true, bool shutdownProperly = false)
	{
		if (queue)
			queue->shutdown(fail_if_not_running, shutdownProperly);
		if (instance)
			instance->shutdown(fail_if_not_running, shutdownProperly);
		if (splitter)
			splitter->shutdown(fail_if_not_running, shutdownProperly);
	}

	/** returns the module instance (if neccessary, it will create it */
	InstanceType* getInstance()
	{
		if (instance != NULL)
			return instance;

		return createInstance();
	}

	//void postReconfiguration() {
	//	instance = getInstance();
	//	instance->postReconfiguration();
	//}

	void onReconfiguration1() {
		if (instance == NULL)
			return;
		instance->onReconfiguration1();
	}

	void onReconfiguration2() {
		if (instance == NULL)
			return;
		instance->onReconfiguration2();
	}

	/** this method gets called *ONLY* if the instance needs a timer and the
	 *  in the configuration there was no timer in front of the instance
	 */
	ConnectionQueue<typename InstanceType::dst_value_type>* getQueueInstance()
	{
		if (!queue) {
			msg(MSG_DIALOG, "queue is required by module id=%u but is not configured. Inserting a default queue with max size 1 (attention: this is inefficient!)", getID());
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
	virtual bool deriveFrom(Cfg* old)
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
		if (!instance || !instanceDeleteAllowed)
			return;

		delete instance;
		instance = NULL;
	}

	/**
	 * tells this module that there aren't any succeeding modules during module connection
	 */
	virtual void setupWithoutSuccessors()
	{
		if (typeid(typename InstanceType::src_value_type)!=typeid(NullEmitable*)) {
			msg(MSG_INFO, "module %s (id=%u) is source for data elements, but has no successor", getName().c_str(), getID());
			getInstance()->connectToNothing();
		}
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
					THROWEXCEPTION("Queue is not a timer; this should never happen. Perhaps modules "
							"with incompatible output/input types are connected in configuration?");

				timer = destQueue;
				dest  = destQueue;
			}
			notifiable->useTimer(timer);
		}

		if (!dest) { // dest wasn't set yet
			dest = dynamic_cast<Destination< typename InstanceType::src_value_type>* >
					(other->getInstance());
			if (!dest) {
				msg(MSG_FATAL, "Trying to connect incompatible types: %s -> %s! Check your configuration for incompabible connections!", this->getName().c_str(), other->getName().c_str());
				THROWEXCEPTION("Unexpected error: can't cast %s to matching Destination<>",
						other->getName().c_str());
			}
		}

		// call postReconfiguration(), e.g. to tell the module to resend its template
		//Gerhard: postReconfiguration() is now called in Module::start()
		//this->postReconfiguration();

		// check if we need a splitter
		if (this->getNext().size() > 1) {
			if (!splitter) {
				splitter = new ConnectionSplitter<typename InstanceType::src_value_type>();
				instance->connectTo(splitter);
			}
			splitter->connectTo(dest);
		} else {
			instance->connectTo(dest);
		}
	}

	/** disconnect the instances and deletes an automaticly created splitter */
	void disconnectInstances()
	{
		// reset the timer to NULL so that the object could be deleted
		Timer* timer = dynamic_cast<Timer*>(instance);
		Notifiable* n = dynamic_cast<Notifiable*>(instance);
		if (timer && n) {
			n->useTimer(NULL);
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

	/** helper method to transfer one instance to another Cfg
	 *  @param other the Cfg entry we want to transfer the instance to
	 */
	inline void transferInstance(Cfg* old)
	{
		ConfigType* cfg = dynamic_cast<ConfigType*>(old);
		if (cfg)
			return this->transferInstance(cfg);

		THROWEXCEPTION("Can't transfer %s from %s",
				this->getName().c_str(), old->getName().c_str());
	}


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

	/** returns this module's instance as a Module
	 * if instance does not inherit a module, NULL is returned
	 */
	Module* getInstanceAsModule()
	{
		Module* m = dynamic_cast<Module*>(instance);
		return m;
	}

protected:

	InstanceType* instance;

	ConnectionSplitter<typename InstanceType::src_value_type>* splitter;
	ConnectionQueue<typename InstanceType::dst_value_type>* queue;

	Notifiable* notifiable;

	std::string name;

private:

	bool instanceDeleteAllowed; /** if set to true, managed module will not be freed automatically */

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
