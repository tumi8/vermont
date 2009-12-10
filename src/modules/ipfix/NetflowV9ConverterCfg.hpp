#ifndef _NETFLOWV9CONVERTERCFG_H_
#define _NETFLOWV9CONVERTERCFG_H_

#include <core/XMLElement.h>
#include <core/Cfg.h>

#include "modules/ipfix/NetflowV9Converter.hpp"

#include <string>

using namespace std;


class NetflowV9ConverterCfg
	: public CfgHelper<NetflowV9Converter, NetflowV9ConverterCfg>
{
public:
	friend class ConfigManager;

	virtual NetflowV9ConverterCfg* create(XMLElement* e);
	virtual ~NetflowV9ConverterCfg();

	virtual NetflowV9Converter* createInstance();
	virtual bool deriveFrom(NetflowV9ConverterCfg* old);

protected:

	bool copyMode;

	NetflowV9ConverterCfg(XMLElement*);
};


#endif
