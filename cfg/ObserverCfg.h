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
	: public CfgHelper<Observer, ObserverCfg>
{
public:
	friend class ConfigManager;

	virtual ObserverCfg* create(XMLElement* e);
	
	virtual ~ObserverCfg();

	virtual Observer* createInstance();

	virtual bool deriveFrom(ObserverCfg* old);
	
protected:
	ObserverCfg(XMLElement*);

private:
	// config variables
	std::string interface;
	std::string pcap_filter;
	unsigned int capture_len;
};

#endif /*OBSERVERCFG_H_*/
