#ifndef PCAPEXPORTERCFG_H_
#define PCAPEXPORTERCFG_H_

#include "Cfg.h"
#include "PCAPExporterModule.h"

#include <vector>


class PCAPExporterCfg
	: public CfgHelper<PCAPExporterModule, PCAPExporterCfg>
{
	friend class ConfigManager;
public:
	virtual ~PCAPExporterCfg();

	virtual PCAPExporterCfg* create(XMLElement* elem);
	
	virtual PCAPExporterModule* createInstance();
	
	bool deriveFrom(PCAPExporterCfg* old);

protected:
	PCAPExporterCfg(XMLElement* elem); 

private:
	std::string fileName;
	int link_type;
};


#endif /*PCAP_EXPORTERCFG_H_*/
