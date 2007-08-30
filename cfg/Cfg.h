#ifndef CFG_H_
#define CFG_H_

#include "cfg/XMLElement.h"
#include "reconf/Module.h"

#include <string>

class Cfg
{
public:
	friend class ConfigFile;

	virtual Cfg* create(XMLElement* e) = 0;
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

	/** return a string value of an elemen */
	std::string get(const std::string& name);

	/** returns the integer value of an XML config entry
	 * If there is no such element in the XML file, it returns def
	 */
	int getInt(const std::string& name, int def = 0);

	XMLElement* _elem;
};

#endif /*CFG_H_*/
