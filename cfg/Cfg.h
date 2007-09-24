#ifndef CFG_H_
#define CFG_H_

#include "cfg/XMLElement.h"
#include "reconf/Module.h"

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
	 */
	virtual Module* getInstance() = 0;

	virtual bool deriveFrom(Cfg*) = 0;

	virtual void connectInstances(Cfg* other)
	{
		getInstance()->connectTo(other->getInstance());
	};

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
	unsigned int getTimeInUnit(const std::string& name, timeUnit unit, XMLElement* elem = NULL);
	
	XMLElement* _elem;
};

#endif /*CFG_H_*/
