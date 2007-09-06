/*
 released under GPL v2
 (C) by Lothar Braun <mail@lobraun.de>
*/


#ifndef _OBSERVER_CONFIGURATION_H_
#define _OBSERVER_CONFIGURATION_H_


#include "ipfix_configuration.h"

#include "sampler/Observer.h"
#include "common/InstanceManager.h"

#include <string>


class Filter;


class ObserverConfiguration : public Configuration {
public:
	ObserverConfiguration(xmlDocPtr document, xmlNodePtr startPoint);
	~ObserverConfiguration();
	
	virtual void configure();
	virtual void connect(Configuration*);
	virtual void startSystem();
	virtual void stopSystem();

protected:
	void setUp();

private:
	unsigned int observationDomain;
	bool offlinemode;
	bool replaceOfflineTimestamps;
	float offlineSpeed;
	std::string type;
	std::string interface;    // used for filename in offline mode
	std::string pcapFilter;
	int captureLength;

	void parseParameters(xmlNodePtr p);

	Observer* observer;
	InstanceManager<Packet> packetManager;
};

#endif
