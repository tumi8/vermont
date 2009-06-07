#ifndef PSAMPEXPORTERCFG_H_
#define PSAMPEXPORTERCFG_H_

#include "core/Cfg.h"
#include "cfg/CollectorCfg.h"
#include "modules/packet/PSAMPExporterModule.h"

#include <vector>

// forward declarations
class PacketReportingCfg;

class PSAMPExporterCfg
	: public CfgHelper<PSAMPExporterModule, PSAMPExporterCfg>
{
	friend class ConfigManager;
public:
	virtual ~PSAMPExporterCfg();

	virtual PSAMPExporterCfg* create(XMLElement* elem);
	
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

	uint32_t observationDomainId;
	
	PacketReportingCfg* reporting;
};


#endif /*PSAMPEXPORTERCFG_H_*/
