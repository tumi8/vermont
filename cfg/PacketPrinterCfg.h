#ifndef PACKETPRINTERCFG_H_
#define PACKETPRINTERCFG_H_

#include "cfg/Cfg.h"
#include "reconf/PrinterModule.h"

#include <cassert>

class PacketPrinterCfg 
	: public Cfg
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
		if (printer)
			return printer;
		
		fprintf(stderr, "INIT PRINTER\n");
		
		bool isEndPoint = false;

		XMLAttribute* attr = _elem->getAttribute("endPoint");
		if (attr && strcasecmp(attr->getValue().c_str(), "true") == 0)
			isEndPoint = true;
		
		if (isEndPoint)
			printf("ENDPOINT\n");
		printer = new PrinterModule(isEndPoint);
		return printer;
	}


protected:
	PacketPrinterCfg(XMLElement* e)	: Cfg(e), printer(NULL) { }
	
private:
	PrinterModule* printer;
};

#endif /*PACKETPRINTERCFG_H_*/
