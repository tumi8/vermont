#ifndef PSAMPEXPORTERCFG_H_
#define PSAMPEXPORTERCFG_H_

#include "Cfg.h"
#include "CollectorCfg.h"
#include "PSAMPExporterModule.h"

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
