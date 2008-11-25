#ifndef _RECORD_ANONYMIZER_CFG_H_
#define _RECORD_ANONYMIZER_CFG_H_

#include <cfg/Cfg.h>

#include <reconf/RecordAnonymizer.h>

class RecordAnonymizerCfg
	: public CfgHelper<RecordAnonymizer, RecordAnonymizerCfg>
{
public:
	friend class ConfigManager;

	virtual RecordAnonymizerCfg* create(XMLElement* e);
	
	virtual ~RecordAnonymizerCfg();

	virtual RecordAnonymizer* createInstance();

	virtual bool deriveFrom(RecordAnonymizerCfg* old);

	static void initInstance(CfgBase* c, AnonModule* module, XMLNode::XMLSet<XMLElement*> set);
	
protected:
	RecordAnonymizerCfg(XMLElement*);
};

#endif
