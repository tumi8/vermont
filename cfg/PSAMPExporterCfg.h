#ifndef PSAMPEXPORTERCFG_H_
#define PSAMPEXPORTERCFG_H_

#include "Cfg.h"
#include "PSAMPExporterModule.h"

#include <vector>

/**
 * This class holds the <collector> ... </collector> information of the config
 */
class CollectorCfg
	: public CfgBase
{
	friend class PSAMPExporterCfg;

	std::string getName() { return "collector"; }
	
protected:
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

	std::string ipAddress;
	unsigned ipAddressType;
	std::string protocolType;
	uint16_t port;
};

// forward declarations
class PacketReportingCfg;

class PSAMPExporterCfg
	: public CfgHelper<PSAMPExporterModule, PSAMPExporterCfg>
{
	friend class ConfigManager;
public:
	virtual ~PSAMPExporterCfg();

	virtual PSAMPExporterCfg* create(XMLElement* elem);
	
	virtual std::string getName() { return "psampExporter"; }
	
	virtual PSAMPExporterModule* createInstance();
	
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
	
	int recordsPerPacket;
	int recordLength;
	
	PacketReportingCfg* reporting;
};


#endif /*PSAMPEXPORTERCFG_H_*/
