#ifndef PCAPEXPORTERCFG_H_
#define PCAPEXPORTERCFG_H_

#include "core/Cfg.h"
#include "reconf/PCAPExporterModule.h"

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
	int snaplen;
};


#endif /*PCAP_EXPORTERCFG_H_*/
