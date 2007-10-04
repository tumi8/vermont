#ifndef CFG_H_
#define CFG_H_

#include "cfg/XMLElement.h"
#include "reconf/Module.h"

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

class Cfg
{
public:
	friend class ConfigFile;


	
	
	virtual Cfg* create(XMLElement* e) { return NULL; };
	virtual ~Cfg() { }

	/** returns the name (as written in the XML file) */
	virtual std::string getName() = 0;

	/** ID given in the XML file */
	unsigned int getID();

	/** returns all the ID's of the next processing elements in the pipeline */
	std::vector<unsigned int> getNext();

	/** returns an instance of the module which the config element describes  
	 *  This uses covariant return types, so every derived class has to implement it
	 *  if it holds a module instance (this is why the method isn't pure virtual)
	 *  @param create true if you want to actually create an instance (default)
	 */
	virtual Module* getInstance()
	{
		THROWEXCEPTION("getInstance not overloaded"); 
		return NULL; // -Wall -Werror in debug mode needs this
	}

	/** returns true if we could reuse the other instance 
	 *  @param other Cfg describing the old/other elemen
	 */
	virtual bool deriveFrom(Cfg* other) = 0;

	/** connectes this module with the module from \other
	 *  @param other the other Cfg
	 */
	virtual void connectInstances(Cfg* other)
	{
		THROWEXCEPTION("connectInstances must be overriden if called\n");
	}

	virtual void disconnectInstances()
	{
		
	}
	
protected:
	Cfg(XMLElement* e) : _elem(e) { }

	/** return a string value of an elemen 
	 * @param name the name of the element
	 */
	std::string get(const std::string& name, XMLElement* elem = NULL) throw(IllegalEntry);

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

/**
 * This is a helper class to avoid code duplication over all the other Cfg subclasses
 * It helps to manage the adapter classes like Splitter, which are otherwise very difficult
 * to represent.
 */
template <typename I>
struct CfgHelper {
	I* instance;
	SplitterAdapter<I>* splitterAdapter;

	CfgHelper() : instance(NULL), splitterAdapter(NULL) { };
	
	~CfgHelper()
	{
		if (splitterAdapter)
			delete splitterAdapter;
	}
	
	void freeInstance()
	{
		if (!instance)
			return;
		
		delete instance;
		instance = NULL;
	}
	
	void connectTo(BaseDestination* other, int ai)
	{
		if ((ai & NEED_TIMEOUT) == NEED_TIMEOUT) {
			msg(MSG_FATAL, "we need a TimoutAdapter; this case isn't handled yet"); // FIXME: here has to be something
			printf("%u\n", ai);
			printf("%u\n", NO_ADAPTER);
		}
	
		if ((ai & NEED_SPLITTER) == NEED_SPLITTER) {
			if (!splitterAdapter)
				splitterAdapter = new SplitterAdapter<I>(instance);
			splitterAdapter->connectTo(other);
		} else
			instance->connectTo(other);
	}

	virtual void connectInstances(Cfg* other)
	{
		printf("CFGHelper::connect called\n");
	}
	
	void disconnect()
	{
		// FIXME: implement the NEED_TIMEOUT case
		
		if (splitterAdapter) {
			splitterAdapter->disconnect();
			freeSplitter();
		}
		instance->disconnect();
	} 

protected:
	inline void transferInstance(CfgHelper* other)
	{
		instance = other->instance;
		other->instance = NULL;
	}
	
private:
	void freeSplitter()
	{
		if (!splitterAdapter)
			return;
		delete splitterAdapter;
		splitterAdapter = NULL;
	}
};


#endif /*CFG_H_*/
