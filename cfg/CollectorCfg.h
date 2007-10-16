#ifndef COLLECTORCFG_H_
#define COLLECTORCFG_H_

#include "Cfg.h"

#include <string>

/**
 * This class holds the <collector> ... </collector> information of the config
 */
class CollectorCfg
	: public CfgBase
{
public:
	std::string getName() { return "collector"; }
	
	CollectorCfg(XMLElement* elem)
		: CfgBase(elem)
	{
		try {
			ipAddress = get("ipAddress");
			protocolType = get("transportProtocol");
			if (protocolType == "17")
				protocolType = "UDP";
			port = (uint16_t)getInt("port", 4739);
		} catch(IllegalEntry ie) {
			THROWEXCEPTION("Illegal Collector entry in config file");
		}
	}

	std::string getIpAddress() { return ipAddress; }
	unsigned getIpAddressType() { return ipAddressType; }
	std::string getProtocolType() { return protocolType; }
	uint16_t getPort() { return port; }
	
private:	
	std::string ipAddress;
	unsigned ipAddressType;
	std::string protocolType;
	uint16_t port;
};

#endif /*COLLECTORCFG_H_*/
