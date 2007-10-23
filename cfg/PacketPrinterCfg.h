#ifndef PACKETPRINTERCFG_H_
#define PACKETPRINTERCFG_H_

#include "cfg/Cfg.h"
#include "reconf/PrinterModule.h"

#include <cassert>

class PacketPrinterCfg 
	: public CfgHelper<PrinterModule, PacketPrinterCfg>
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
	
	virtual PrinterModule* createInstance()
	{
		instance = new PrinterModule(isEndPoint);
		
		std::string s;
		char ca[10];
		snprintf(ca, 10, "%d", getID());

		instance->setPrefix(ca);
		
		return instance;
	}

	virtual bool deriveFrom(PacketPrinterCfg* old)
	{
		if (isEndPoint != old->isEndPoint)
			return false;
		
		transferInstance(old);
		return true;
	}

protected:
	PacketPrinterCfg(XMLElement* e)	
		: CfgHelper<PrinterModule, PacketPrinterCfg>(e, "packetPrinter"),
		isEndPoint(false)
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
