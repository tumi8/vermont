#ifndef PSAMPEXPORTERCFG_H_
#define PSAMPEXPORTERCFG_H_

#include "Cfg.h"
#include "PSAMPExporterModule.h"

#include <vector>

/**
 * This class holds the <collector> ... </collector> information of the config
 */
class CollectorCfg
	: public Cfg
{
	friend class PSAMPExporterCfg;
	
	bool deriveFrom(Cfg* old);
		
	bool deriveFrom(CollectorCfg* old);

	std::string getName() { return "collector"; }
	
	// not used
	Module* getInstance() { return NULL; };
	
protected:
	CollectorCfg(XMLElement* elem)
		: Cfg(elem)
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

	std::string ipAddress;
	unsigned ipAddressType;
	std::string protocolType;
	uint16_t port;
};

// forward declarations
class PacketReportingCfg;

class PSAMPExporterCfg
	: public Cfg
{
	friend class ConfigManager;
public:
	virtual ~PSAMPExporterCfg();

	virtual PSAMPExporterCfg* create(XMLElement* elem);
	
	virtual std::string getName() { return"psampExporter"; }
	
	//Module* getInstance();
	virtual PSAMPExporterModule* getInstance();
	
	bool deriveFrom(Cfg* old);
		
	bool deriveFrom(PSAMPExporterCfg* old);
	
protected:
	PSAMPExporterCfg(XMLElement* elem); 

private:
	/** this holds the info for all the hosts we want to export to */
	std::vector<CollectorCfg*> collectors;

	/** udpTemplateManagement */
	unsigned templateRefreshTime;
	unsigned templateRefreshRate;

	/** packet restrictions */
	uint16_t maxPacketSize;
	unsigned exportDelay;
	uint16_t capture_len;
	
	int recordsPerPacket;
	int recordLength;
	
	PacketReportingCfg* reporting;
	
	PSAMPExporterModule* instance;
};


#endif /*PSAMPEXPORTERCFG_H_*/
