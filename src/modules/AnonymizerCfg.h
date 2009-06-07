#ifndef _RECORD_ANONYMIZER_CFG_H_
#define _RECORD_ANONYMIZER_CFG_H_

#include <core/Cfg.h>

#include <modules/ipfix/IpfixRecordAnonymizer.h>

class AnonymizerCfg
	: public CfgHelper<IpfixRecordAnonymizer, AnonymizerCfg>
{
public:
	friend class ConfigManager;

	virtual AnonymizerCfg* create(XMLElement* e);

	virtual ~AnonymizerCfg();

	virtual IpfixRecordAnonymizer* createInstance();

	virtual bool deriveFrom(AnonymizerCfg* old);

	static void initInstance(CfgBase* c, AnonModule* module, XMLNode::XMLSet<XMLElement*> set);

protected:
	AnonymizerCfg(XMLElement*);
};

#endif
