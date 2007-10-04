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
	: public Cfg, CfgHelper<PSAMPExporterModule>
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
};


#endif /*PSAMPEXPORTERCFG_H_*/
