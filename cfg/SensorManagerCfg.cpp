#include "SensorManagerCfg.h"

#include <cassert>

#include <stdlib.h>

SensorManagerCfg::SensorManagerCfg(XMLElement* elem)
	: CfgHelper<SensorManager, SensorManagerCfg>(elem, "sensorManager"),
	  checkInterval(SM_DEFAULT_CHECK_INTERVAL),
	  sensorOutput(SM_DEFAULT_OUTPUT_FNAME)
{
	if (!elem) return; // needed because of table inside ConfigManager
	
	XMLNode::XMLSet<XMLElement*> set = _elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("check_interval")) {
			checkInterval = strtol(e->getFirstText().c_str(), NULL, 10);
			if (checkInterval == 0) {
				THROWEXCEPTION("invalid check interval specified: '%s'", e->getFirstText().c_str());
			}
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
	instance = new SensorManager(checkInterval, sensorOutput, graphIS);
	
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
