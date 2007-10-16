#ifndef IPFIXEXPORTERCFG_H_
#define IPFIXEXPORTERCFG_H_

#include "Cfg.h"
#include "CollectorCfg.h"
#include "concentrator/IpfixSender.hpp"

class IpfixExporterCfg
	: public CfgHelper<IpfixSender, IpfixExporterCfg>
{
	friend class ConfigManager;
public:
	virtual ~IpfixExporterCfg();

	virtual IpfixExporterCfg* create(XMLElement* elem);

	virtual std::string getName() { return "ipfixExporter"; }

	virtual IpfixSender* createInstance();

	bool deriveFrom(IpfixExporterCfg* old);

protected:
	IpfixExporterCfg(XMLElement* elem);

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
};

#endif /*IPFIXEXPORTERCFG_H_*/
