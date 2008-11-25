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

	virtual IpfixSender* createInstance();

	bool deriveFrom(IpfixExporterCfg* old);
	
	bool equalTo(IpfixExporterCfg* other);

protected:
	IpfixExporterCfg(XMLElement* elem);

private:
	/** this holds the info for all the hosts we want to export to */
	std::vector<CollectorCfg*> collectors;

	/** template management */
	unsigned templateRefreshTime;
	unsigned templateRefreshRate;
	
	/** sctp parameters */
	uint32_t sctpDataLifetime;
	uint32_t sctpReconnectInterval;

	/** packet restrictions */
	uint16_t maxPacketSize;
	unsigned exportDelay;
	
	uint32_t recordRateLimit;
	uint32_t observationDomainId;
	
	int recordsPerPacket;
	int recordLength;
};

#endif /*IPFIXEXPORTERCFG_H_*/
