#ifndef PACKETPRINTERCFG_H_
#define PACKETPRINTERCFG_H_

#include "cfg/Cfg.h"
#include "reconf/PrinterModule.h"

#include <cassert>

class PacketPrinterCfg 
	: public Cfg, CfgHelper<PrinterModule>
{
public:
	friend class ConfigManager;
	
	virtual PacketPrinterCfg* create(XMLElement* e)
	{
		assert(e);
		assert(e->getName() == getName());
		
		return new PacketPrinterCfg(e);
	}
	
	virtual ~PacketPrinterCfg() {};

	virtual std::string getName() { return "packetPrinter"; }
	
	virtual PrinterModule* getInstance()
	{
		if (instance)
			return instance;
		

		instance = new PrinterModule(isEndPoint);
		return instance;
	}

	virtual bool deriveFrom(Cfg* old)
	{
		PacketPrinterCfg* cfg = dynamic_cast<PacketPrinterCfg*>(old);
		if (cfg)
			return deriveFrom(cfg);

		THROWEXCEPTION("Derive is only allowed from within the same type");
		return false;
	}

	virtual bool deriveFrom(PacketPrinterCfg* old)
	{
		if (isEndPoint != old->isEndPoint)
			return false;
		
		instance = old->getInstance();
		return true;
	}

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
	PacketPrinterCfg(XMLElement* e)	: Cfg(e), isEndPoint(false)
	{
		if (!e)
			return;

		XMLAttribute* attr = _elem->getAttribute("endPoint");
		if (attr && strcasecmp(attr->getValue().c_str(), "true") == 0)
			isEndPoint = true;
		
		if (isEndPoint)
			printf("ENDPOINT\n");
	}
	
private:
	bool isEndPoint;
};

#endif /*PACKETPRINTERCFG_H_*/
