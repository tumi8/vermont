#include "SensorManagerCfg.h"

#include <cassert>

#include <stdlib.h>

bool SensorManagerCfg::instanceCreated = false;

SensorManagerCfg::SensorManagerCfg(XMLElement* elem)
	: CfgHelper<SensorManager, SensorManagerCfg>(elem, "sensorManager", false),
	  checkInterval(SM_DEFAULT_CHECK_INTERVAL),
	  sensorOutput(SM_DEFAULT_OUTPUT_FNAME),
	  append(SM_DEFAULT_APPEND)
{
	if (!elem) return; // needed because of table inside ConfigManager
	
	XMLNode::XMLSet<XMLElement*> set = _elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("checkinterval")) {
			checkInterval = strtol(e->getFirstText().c_str(), NULL, 10);
			if (checkInterval == 0) {
				THROWEXCEPTION("invalid check interval specified: '%s'", e->getFirstText().c_str());
			}
		} else if (e->matches("outputfile")) {
			sensorOutput = e->getFirstText().c_str();
			if (sensorOutput.size() == 0) {
				THROWEXCEPTION("invalid sensor outut file specified: '%s'", e->getFirstText().c_str());
			}
		} else if (e->matches("append")) {
			append = getInt("append")>0;
		} else {
			msg(MSG_FATAL, "Unknown sensor manager config statement: %s", e->getName().c_str());
		}
	}
}

SensorManagerCfg::~SensorManagerCfg()
{
}

SensorManagerCfg* SensorManagerCfg::create(XMLElement* e)
{
	assert(e);
	assert(e->getName() == getName());
	return new SensorManagerCfg(e);
}

SensorManager* SensorManagerCfg::createInstance()
{
	assert(graphIS);
	if (instanceCreated) {
		THROWEXCEPTION("multiple instances of module SensorManager must not be created");
	}
	instance = &SensorManager::getInstance();
	instance->setCheckInterval(checkInterval);
	instance->setOutputFilename(sensorOutput);
	instance->setGraphIS(graphIS);
	instance->setAppend(append);
	instanceCreated = true;
	return instance;
}

bool SensorManagerCfg::deriveFrom(SensorManagerCfg* old)
{
	if (checkInterval != old->checkInterval) return false;
	return true;
}

void SensorManagerCfg::setGraphIS(GraphInstanceSupplier* gis)
{
	graphIS = gis;
	if (instance) instance->setGraphIS(gis);
}
