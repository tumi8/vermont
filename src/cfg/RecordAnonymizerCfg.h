#ifndef _RECORD_ANONYMIZER_CFG_H_
#define _RECORD_ANONYMIZER_CFG_H_

#include <core/Cfg.h>

#include <reconf/IpfixRecordAnonymizer.h>

class RecordAnonymizerCfg
	: public CfgHelper<IpfixRecordAnonymizer, RecordAnonymizerCfg>
{
public:
	friend class ConfigManager;

	virtual RecordAnonymizerCfg* create(XMLElement* e);

	virtual ~RecordAnonymizerCfg();

	virtual IpfixRecordAnonymizer* createInstance();

	virtual bool deriveFrom(RecordAnonymizerCfg* old);

	static void initInstance(CfgBase* c, AnonModule* module, XMLNode::XMLSet<XMLElement*> set);

protected:
	RecordAnonymizerCfg(XMLElement*);
};

#endif
