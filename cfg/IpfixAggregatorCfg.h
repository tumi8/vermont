#ifndef FLOWMETERINGCFG_H_
#define FLOWMETERINGCFG_H_

#include "AggregatorBaseCfg.h"
#include "Cfg.h"
#include "concentrator/IpfixAggregator.hpp"

class IpfixAggregatorCfg
	: public CfgHelper<IpfixAggregator, IpfixAggregatorCfg>, public AggregatorBaseCfg
{
public:
	friend class ConfigManager;
	
	virtual ~IpfixAggregatorCfg();

	virtual IpfixAggregatorCfg* create(XMLElement* elem);
	
	virtual IpfixAggregator* createInstance();
	
	bool deriveFrom(IpfixAggregatorCfg* old);


protected:
	IpfixAggregatorCfg(XMLElement* elem);
	
};

#endif /*FLOWMETERINGCFG_H_*/
