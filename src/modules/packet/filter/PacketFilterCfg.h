/*
 * Vermont Packet Filter
 * Copyright (C) 2009  Vermont Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef PACKETFILTERCFG_H_
#define PACKETFILTERCFG_H_

#include <core/Cfg.h>
#include "modules/packet/filter/FilterModule.h"

#include <set>
#include <vector>
#include <algorithm>

// forward declaration of instances
class RegExFilter;
class HostFilter;
class StringFilter;
class SystematicSampler;
class StateConnectionFilter;
class ConnectionFilter;
class AnonFilter;
class PayloadFilter;

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

class PacketFilterHelperCfg
	: public Cfg
{

	/* we have to implement those, because from an implementation standpoint
	 * the filters could be modules of its own, but as discussed, they where just
	 * subparts of a bigger FilterModule
	 */
	virtual void transferInstance(Cfg* other) { THROWEXCEPTION("Not supported"); }
	virtual void start(bool fail_if_already_running = true) { THROWEXCEPTION("Not supported"); }
	virtual void shutdown(bool fail_if_not_running = true, bool shutdownProperly = false) { THROWEXCEPTION("Not supported"); }
	virtual void postReconfiguration() { THROWEXCEPTION("Not supported"); }
	virtual void onReconfiguration1() { THROWEXCEPTION("Not supported"); }
	virtual void onReconfiguration2() { THROWEXCEPTION("Not supported"); }
	virtual void freeInstance() { THROWEXCEPTION("Not supported"); }
	virtual void connectInstances(Cfg* other) { THROWEXCEPTION("Not supported"); }
	virtual void setupWithoutSuccessors() { THROWEXCEPTION("Not supported"); }
	virtual void disconnectInstances()  { THROWEXCEPTION("Not supported"); }
	virtual Module* getQueueInstance()  { THROWEXCEPTION("Not supported"); return NULL; }
protected:
	PacketFilterHelperCfg(XMLElement *e);
};

class HostFilterCfg
	: public PacketFilterHelperCfg
{
public:
	friend class PacketFilterCfg;

	virtual PacketFilterCfg* create(XMLElement* e) {return NULL; };

	virtual ~HostFilterCfg();

	virtual std::string getName() { return "hostBased"; }

	virtual std::string getAddrFilter() { return addrFilter; }
	virtual std::set<uint32_t> getIpList();

	virtual Module* getInstance();

	virtual bool deriveFrom(Cfg* old)
	{
		HostFilterCfg* cfg = dynamic_cast<HostFilterCfg*>(old);
		if (cfg)
			return deriveFrom(cfg);

		THROWEXCEPTION("Can't derive from HostFilter");
		return false;
	}

	virtual bool deriveFrom(HostFilterCfg* old)
	{
		if ((getIpList() != old->getIpList()) || (getAddrFilter() != old->getAddrFilter()))
			return false;

		return true;
	}
protected:
	HostFilterCfg(XMLElement *e);

private:
	HostFilter* instance;
	std::string addrFilter;
	std::set<uint32_t> ipList;
};

class PacketCountFilterCfg
	: public PacketFilterHelperCfg
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


class PacketTimeFilterCfg
	: public PacketFilterHelperCfg
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
protected:
	PacketTimeFilterCfg(XMLElement *e);

private:
	SystematicSampler* instance;
};


class PacketStringFilterCfg
	: public PacketFilterHelperCfg
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
	PacketStringFilterCfg(XMLElement *e) : PacketFilterHelperCfg(e), instance(NULL)
	{
	}


private:
	StringFilter* instance;
};


class PacketRegexFilterCfg
	: public PacketFilterHelperCfg
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
protected:
	PacketRegexFilterCfg(XMLElement *e): PacketFilterHelperCfg(e), instance(NULL) { };

private:
	RegExFilter* instance;
};


class PacketStateConnectionFilterCfg
	: public PacketFilterHelperCfg
{
public:
	friend class PacketFilterCfg;

	virtual PacketFilterCfg* create(XMLElement* e) {return NULL; };

	virtual ~PacketStateConnectionFilterCfg() { };

	virtual std::string getName() { return "stateConnectionBased"; }

	virtual Module* getInstance();

	virtual bool deriveFrom(Cfg* old)
	{
		PacketStateConnectionFilterCfg* cfg = dynamic_cast<PacketStateConnectionFilterCfg*>(old);
		if (cfg)
			return deriveFrom(cfg);

		THROWEXCEPTION("Can't derive from PacketStateConnectionFilter");
		return false;
	}

	virtual bool deriveFrom(PacketStateConnectionFilterCfg* old);
protected:
	PacketStateConnectionFilterCfg(XMLElement *e): PacketFilterHelperCfg(e), instance(NULL) { };

private:
	StateConnectionFilter* instance;
};

#ifdef HAVE_CONNECTION_FILTER
class PacketConnectionFilterCfg
	: public PacketFilterHelperCfg
{
public:
	friend class PacketFilterCfg;

	virtual PacketFilterCfg* create(XMLElement* e) {return NULL; };

	virtual ~PacketConnectionFilterCfg() { };

	virtual std::string getName() { return "connectionBased"; }

	virtual Module* getInstance();

	virtual bool deriveFrom(Cfg* old)
	{
		PacketConnectionFilterCfg* cfg = dynamic_cast<PacketConnectionFilterCfg*>(old);
		if (cfg)
			return deriveFrom(cfg);

		THROWEXCEPTION("Can't derive from PacketConnectionFilter");
		return false;
	}

	virtual bool deriveFrom(PacketConnectionFilterCfg* old);
protected:
	PacketConnectionFilterCfg(XMLElement *e): PacketFilterHelperCfg(e), instance(NULL) { };

private:
	ConnectionFilter* instance;
};
#endif

class PacketPayloadFilterCfg
	: public PacketFilterHelperCfg
{
public:
	friend class PacketFilterCfg;

	virtual PacketPayloadFilterCfg* create(XMLElement* e) {return NULL; };

	virtual ~PacketPayloadFilterCfg() { };

	virtual std::string getName() { return "anonFilter"; }

	virtual Module* getInstance();

	virtual bool deriveFrom(Cfg* old)
	{
		PacketPayloadFilterCfg* cfg = dynamic_cast<PacketPayloadFilterCfg*>(old);
		if (cfg)
			return deriveFrom(cfg);

		THROWEXCEPTION("Can't derive from PacketPayloadFilter");
		return false;
	}

	virtual bool deriveFrom(PacketPayloadFilterCfg* old);
protected:
	PacketPayloadFilterCfg(XMLElement *e): PacketFilterHelperCfg(e), instance(NULL) { };
private:
	PayloadFilter* instance;
};


class PacketAnonFilterCfg
	: public PacketFilterHelperCfg
{
public:
	friend class PacketFilterCfg;

	virtual PacketAnonFilterCfg* create(XMLElement* e) {return NULL; };

	virtual ~PacketAnonFilterCfg() { };

	virtual std::string getName() { return "anonFilter"; }

	virtual Module* getInstance();

	virtual bool deriveFrom(Cfg* old)
	{
		PacketAnonFilterCfg* cfg = dynamic_cast<PacketAnonFilterCfg*>(old);
		if (cfg)
			return deriveFrom(cfg);

		THROWEXCEPTION("Can't derive from PacketAnonFilter");
		return false;
	}

	virtual bool deriveFrom(PacketAnonFilterCfg* old);
protected:
	PacketAnonFilterCfg(XMLElement *e): PacketFilterHelperCfg(e), instance(NULL) { };
private:
	AnonFilter* instance;
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
		std::transform(ret.begin(), ret.end(), ret.begin(), (int(*)(int))std::tolower);
		return  ret;
	}
	std::string getIeId() { return get("ieId"); }
	std::string getIeLength() { return get("ieLength"); }
	std::string getMatch() { return get("match"); }
	std::string getEnterpriseNumber() { return get("enterpriseNumber"); }
	std::string getModifier() { return get("modifier"); }
};

#endif /*PACKETFILTERCFG_H_*/
