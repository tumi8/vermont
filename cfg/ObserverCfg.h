#ifndef OBSERVERCFG_H_
#define OBSERVERCFG_H_

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <cfg/XMLElement.h>
#include <cfg/Cfg.h>

#include <sampler/Observer.h>

#include <common/InstanceManager.h>
#include <map>

class Observer;

class ObserverCfg
	: public Cfg
{
public:
	friend class ConfigManager;
	
	virtual ObserverCfg* create(XMLElement* e);
	virtual ~ObserverCfg();

	virtual std::string getName() { return "observer"; }
	
	virtual Observer* getInstance();
	
protected:
	ObserverCfg(XMLElement*);

private:
	Observer* observer;
	InstanceManager<Packet> packetManager;
};

#endif /*OBSERVERCFG_H_*/
