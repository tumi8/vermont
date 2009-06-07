#ifndef IPFIXCOLLECTORCFG_H_
#define IPFIXCOLLECTORCFG_H_

#include "core/Cfg.h"
#include "CollectorCfg.h"
#include <concentrator/IpfixCollector.hpp>

class IpfixCollectorCfg
	: public CfgHelper<IpfixCollector, IpfixCollectorCfg>
{
public:
	IpfixCollectorCfg(XMLElement* elem);
	virtual ~IpfixCollectorCfg();
	
	virtual IpfixCollectorCfg* create(XMLElement* elem);
	
	virtual IpfixCollector* createInstance();
	
	bool deriveFrom(IpfixCollectorCfg* old);
	
private:
	CollectorCfg* listener;
	
        IpfixCollector* ipfixCollector;

        uint16_t observationDomainId;
};

#endif /*IPFIXCOLLECTORCFG_H_*/
