/*
 released under GPL v2
 (C) by Lothar Braun <mail@lobraun.de>
*/


#ifndef _VERMONTMAIN_CONFIGURATION_H_
#define _VERMONTMAIN_CONFIGURATION_H_


#include "ipfix_configuration.h"

#include <string>


class VermontMainConfiguration : public Configuration {
public:
	VermontMainConfiguration(xmlDocPtr document, xmlNodePtr startPoint);
	~VermontMainConfiguration();
		
	virtual void configure();
	virtual void startSystem();
	virtual void stopSystem();
	virtual void connect(Configuration*);

	unsigned getPollInterval() { return poll_interval; }
	
protected:
	virtual void setUp() {}

	unsigned poll_interval;
	std::string logfile;
	unsigned log_interval;
};

#endif
