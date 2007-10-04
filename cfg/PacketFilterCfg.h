#ifndef PACKETFILTERCFG_H_
#define PACKETFILTERCFG_H_

#include <cfg/Cfg.h>
#include "reconf/FilterModule.h"

#include <vector>

// forward declaration of instances
class regExFilter;
class stringFilter;
class SystematicSampler;

class PacketFilterCfg
	: public Cfg, CfgHelper<FilterModule>
{
public:
	friend class ConfigManager;
	
	virtual PacketFilterCfg* create(XMLElement* e);
	virtual ~PacketFilterCfg();

	virtual std::string getName() { return "filter"; }
	
	virtual FilterModule* getInstance();
	
	virtual bool deriveFrom(Cfg* old)
	{
		PacketFilterCfg* cfg = dynamic_cast<PacketFilterCfg*>(old);
		if (cfg)
			return deriveFrom(cfg);

		THROWEXCEPTION("Derive is only allowed from within the same type");
		return false;
	}

	virtual bool deriveFrom(PacketFilterCfg* old);
	
	virtual void connectInstances(Cfg* other)
	{
		instance = getInstance();

		int need_adapter = 0;
		need_adapter |= ((getNext().size() > 1) ? NEED_SPLITTER : NO_ADAPTER);

		if ((dynamic_cast<Notifiable*>(other->getInstance()) != NULL) &&
		    (dynamic_cast<Timer*>(instance) == NULL))
			need_adapter |= NEED_TIMEOUT;
		
		connectTo(other->getInstance(), need_adapter);
	}

protected:
	PacketFilterCfg(XMLElement* e);

private:
	std::vector<Cfg*> subCfgs;
};

class PacketCountFilterCfg
	: public Cfg
{
public:
	friend class PacketFilterCfg;
	
	virtual PacketFilterCfg* create(XMLElement* e) {return NULL; };
	
	virtual ~PacketCountFilterCfg();

	virtual std::string getName() { return "countBased"; }
	
	int getInterval() { return getInt("interval", 0); }
	int getSpacing()  { return getInt("spacing", 0); }

	virtual Module* getInstance();
	
	virtual bool deriveFrom(Cfg* old)
	{
		PacketCountFilterCfg* cfg = dynamic_cast<PacketCountFilterCfg*>(old);
		if (cfg)
			return deriveFrom(cfg);

		THROWEXCEPTION("Can't derive from PacketCountFilter");
		return false;
	}

	virtual bool deriveFrom(PacketCountFilterCfg* old)
	{
		if (getInterval() != old->getInterval())
			return false;

		if (getSpacing() != old->getSpacing())
			return false;

		return true;
	}

protected:
	PacketCountFilterCfg(XMLElement *e);

private:
	SystematicSampler* instance;
};


class PacketStringFilterCfg
	: public Cfg
{
public:
	friend class PacketFilterCfg;

	virtual PacketFilterCfg* create(XMLElement* e) {return NULL; };

	virtual ~PacketStringFilterCfg() { };

	virtual std::string getName() { return "stringBased"; }

	virtual Module* getInstance();

	virtual bool deriveFrom(Cfg* old)
	{
		PacketStringFilterCfg* cfg = dynamic_cast<PacketStringFilterCfg*>(old);
		if (cfg)
			return deriveFrom(cfg);

		THROWEXCEPTION("Can't derive from PacketStringFilter");
		return false;
	}

	virtual bool deriveFrom(PacketStringFilterCfg* old);

protected:
	PacketStringFilterCfg(XMLElement *e) : Cfg(e), instance(NULL)
	{
	}


private:
	stringFilter* instance;
};


class PacketRegexFilterCfg
	: public Cfg
{
public:
	friend class PacketFilterCfg;

	virtual PacketFilterCfg* create(XMLElement* e) {return NULL; };

	virtual ~PacketRegexFilterCfg() { };

	virtual std::string getName() { return "regexBased"; }

	virtual Module* getInstance();

	virtual bool deriveFrom(Cfg* old)
	{
		PacketStringFilterCfg* cfg = dynamic_cast<PacketStringFilterCfg*>(old);
		if (cfg)
			return deriveFrom(cfg);

		THROWEXCEPTION("Can't derive from PacketStringFilter");
		return false;
	}

	virtual bool deriveFrom(PacketRegexFilterCfg* old);

protected:
	PacketRegexFilterCfg(XMLElement *e): Cfg(e), instance(NULL) { };

private:
	regExFilter* instance;
};






class InfoElementId
	: public Cfg

{
public:
	InfoElementId(XMLElement* e)
		: Cfg(e)
	{

	}

	/*
		for (XMLNode::XMLNodeSet::iterator i = xmlNodePtr i = p->xmlChildrenNode;
		while (NULL != i) {
			if (m.tagMatches(i, "ieName")) {
				ieName = m.getContent(i);
			} else if (m.tagMatches(i, "match")) {
				match = m.getContent(i);
			} else if (m.tagMatches(i, "modifier")) {
				modifier = m.getContent(i);
			} else if (m.tagMatches(i, "ieId")) {
				ieId = m.getContent(i);
			} else if (m.tagMatches(i, "ieLength")) {
				ieLength = m.getContent(i);
			} else if (m.tagMatches(i, "enterpriseNumber")) {
				enterpriseNumber = m.getContent(i);
			}
			i = i->next;
		}
		std::transform(ieName.begin(), ieName.end(), ieName.begin(), std::tolower);
	}
	 */

	std::string getIeName() {
		std::string ret = get("ieName");
		std::transform(ret.begin(), ret.end(), ret.begin(), std::tolower);
		return  ret;
	}
	std::string getIeId() { return get("ieId"); }
	std::string getIeLength() { return get("ieLength"); }
	std::string getMatch() { return get("match"); }
	std::string getEnterpriseNumber() { return get("enterpriseNumber"); }
	std::string getModifier() { return get("modifier"); }
};

#endif /*PACKETFILTERCFG_H_*/
