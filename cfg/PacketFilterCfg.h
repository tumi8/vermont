#ifndef PACKETFILTERCFG_H_
#define PACKETFILTERCFG_H_

#include <cfg/Cfg.h>
#include "reconf/FilterModule.h"

#include <vector>

// forward declaration of instances
class RegExFilter;
class StringFilter;
class SystematicSampler;

class PacketFilterCfg
	: public CfgHelper<FilterModule, PacketFilterCfg>
{
public:
	friend class ConfigManager;
	
	virtual PacketFilterCfg* create(XMLElement* e);
	virtual ~PacketFilterCfg();

	virtual FilterModule* createInstance();
	
	virtual bool deriveFrom(PacketFilterCfg* old);

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

	/* we have to implement those, because from an implementation standpoint
	 * the filters could be modules of its own, but as discussed, they where just
	 * subparts of a bigger FilterModule
	 */
	virtual void transferInstance(Cfg* other) { THROWEXCEPTION("Not supported"); }
	virtual void start(bool fail_if_already_running = true) { THROWEXCEPTION("Not supported"); }
	virtual void shutdown(bool fail_if_not_running = true) { THROWEXCEPTION("Not supported"); }
	virtual void postReconfiguration() { THROWEXCEPTION("Not supported"); }
	virtual void onReconfiguration1() { THROWEXCEPTION("Not supported"); }
	virtual void onReconfiguration2() { THROWEXCEPTION("Not supported"); }
	virtual void freeInstance() { THROWEXCEPTION("Not supported"); }
	virtual void connectInstances(Cfg* other) { THROWEXCEPTION("Not supported"); }
	virtual void disconnectInstances()  { THROWEXCEPTION("Not supported"); }
	virtual Module* getQueueInstance()  { THROWEXCEPTION("Not supported"); return NULL; }
	
	
protected:
	PacketCountFilterCfg(XMLElement *e);

private:
	SystematicSampler* instance;
};


class PacketTimeFilterCfg
	: public Cfg
{
public:
	friend class PacketFilterCfg;
	
	virtual PacketFilterCfg* create(XMLElement* e) {return NULL; };
	
	virtual ~PacketTimeFilterCfg();

	virtual std::string getName() { return "timeBased"; }
	
	int getInterval() { return getInt("interval", 0); }
	int getSpacing()  { return getInt("spacing", 0); }

	virtual Module* getInstance();
	
	virtual bool deriveFrom(Cfg* old)
	{
		PacketTimeFilterCfg* cfg = dynamic_cast<PacketTimeFilterCfg*>(old);
		if (cfg)
			return deriveFrom(cfg);

		THROWEXCEPTION("Can't derive from PacketTimeFilter");
		return false;
	}

	virtual bool deriveFrom(PacketTimeFilterCfg* old)
	{
		if (getInterval() != old->getInterval())
			return false;

		if (getSpacing() != old->getSpacing())
			return false;

		return true;
	}

	/* we have to implement those, because from an implementation standpoint
	 * the filters could be modules of its own, but as discussed, they where just
	 * subparts of a bigger FilterModule
	 */
	virtual void transferInstance(Cfg* other) { THROWEXCEPTION("Not supported"); }
	virtual void start(bool fail_if_already_running = true) { THROWEXCEPTION("Not supported"); }
	virtual void shutdown(bool fail_if_not_running = true) { THROWEXCEPTION("Not supported"); }
	virtual void postReconfiguration() { THROWEXCEPTION("Not supported"); }
	virtual void onReconfiguration1() { THROWEXCEPTION("Not supported"); }
	virtual void onReconfiguration2() { THROWEXCEPTION("Not supported"); }
	virtual void freeInstance() { THROWEXCEPTION("Not supported"); }
	virtual void connectInstances(Cfg* other) { THROWEXCEPTION("Not supported"); }
	virtual void disconnectInstances()  { THROWEXCEPTION("Not supported"); }
	virtual Module* getQueueInstance()  { THROWEXCEPTION("Not supported"); return NULL; }
	
	
protected:
	PacketTimeFilterCfg(XMLElement *e);

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

	/* we have to implement those, because from an implementation standpoint
	 * the filters could be modules of its own, but as discussed, they where just
	 * subparts of a bigger FilterModule
	 */
	virtual void transferInstance(Cfg* other) { THROWEXCEPTION("Not supported"); }
	virtual void start(bool fail_if_already_running = true) { THROWEXCEPTION("Not supported"); }
	virtual void shutdown(bool fail_if_not_running = true) { THROWEXCEPTION("Not supported"); }
	virtual void postReconfiguration() { THROWEXCEPTION("Not supported"); }
	virtual void onReconfiguration1() { THROWEXCEPTION("Not supported"); }
	virtual void onReconfiguration2() { THROWEXCEPTION("Not supported"); }
	virtual void freeInstance() { THROWEXCEPTION("Not supported"); }
	virtual void connectInstances(Cfg* other) { THROWEXCEPTION("Not supported"); }
	virtual void disconnectInstances()  { THROWEXCEPTION("Not supported"); }
	virtual Module* getQueueInstance()  { THROWEXCEPTION("Not supported"); return NULL; }
	
protected:
	PacketStringFilterCfg(XMLElement *e) : Cfg(e), instance(NULL)
	{
	}


private:
	StringFilter* instance;
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
		PacketRegexFilterCfg* cfg = dynamic_cast<PacketRegexFilterCfg*>(old);
		if (cfg)
			return deriveFrom(cfg);

		THROWEXCEPTION("Can't derive from PacketRegexFilter");
		return false;
	}

	virtual bool deriveFrom(PacketRegexFilterCfg* old);

	/* we have to implement those, because from an implementation standpoint
	 * the filters could be modules of its own, but as discussed, they where just
	 * subparts of a bigger FilterModule
	 */
	virtual void transferInstance(Cfg* other) { THROWEXCEPTION("Not supported"); }
	virtual void start(bool fail_if_already_running = true) { THROWEXCEPTION("Not supported"); }
	virtual void shutdown(bool fail_if_not_running = true) { THROWEXCEPTION("Not supported"); }
	virtual void postReconfiguration() { THROWEXCEPTION("Not supported"); }
	virtual void onReconfiguration1() { THROWEXCEPTION("Not supported"); }
	virtual void onReconfiguration2() { THROWEXCEPTION("Not supported"); }
	virtual void freeInstance() { THROWEXCEPTION("Not supported"); }
	virtual void connectInstances(Cfg* other) { THROWEXCEPTION("Not supported"); }
	virtual void disconnectInstances()  { THROWEXCEPTION("Not supported"); }
	virtual Module* getQueueInstance()  { THROWEXCEPTION("Not supported"); return NULL; }
	
protected:
	PacketRegexFilterCfg(XMLElement *e): Cfg(e), instance(NULL) { };

private:
	RegExFilter* instance;
};






class InfoElementId
	: public CfgBase

{
public:
	InfoElementId(XMLElement* e)
		: CfgBase(e)
	{

	}

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
