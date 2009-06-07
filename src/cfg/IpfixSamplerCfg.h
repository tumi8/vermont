#ifndef IPFIXSAMPLERCFG_H_
#define IPFIXSAMPLERCFG_H_

#include <cfg/XMLElement.h>
#include <cfg/Cfg.h>

#include "concentrator/IpfixSampler.h"

#include <string>

using namespace std;


class IpfixSamplerCfg
	: public CfgHelper<IpfixSampler, IpfixSamplerCfg>
{
public:
	friend class ConfigManager;

	virtual IpfixSamplerCfg* create(XMLElement* e);
	virtual ~IpfixSamplerCfg();

	virtual IpfixSampler* createInstance();
	virtual bool deriveFrom(IpfixSamplerCfg* old);

protected:

	double flowRate;

	IpfixSamplerCfg(XMLElement*);
};


#endif
