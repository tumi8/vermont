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

	virtual bool deriveFrom(Cfg* old)
	{
		ObserverCfg* cfg = dynamic_cast<ObserverCfg*>(old);
		if (cfg)
			return deriveFrom(cfg);

		THROWEXCEPTION("Derive is only allowed from within the same type");
		return false;
	}

	virtual bool deriveFrom(ObserverCfg* old);

protected:
	ObserverCfg(XMLElement*);

private:
	Observer* observer;

	// config variables
	std::string interface;
	std::string pcap_filter;
	unsigned int capture_len;
};

#endif /*OBSERVERCFG_H_*/
