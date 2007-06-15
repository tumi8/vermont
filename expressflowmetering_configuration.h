/*
 released under GPL v2
 (C) by Lothar Braun <mail@lobraun.de>
*/

#ifndef   	EXPRESSFLOWMETERING_CONFIGURATION_H_
#define   	EXPRESSFLOWMETERING_CONFIGURATION_H_


#include "ipfix_configuration.h"


#include <concentrator/IpfixAggregator.hpp>
#include <concentrator/Rules.hpp>


class MeteringConfiguration;


class ExpressFlowMeteringConfiguration : public Configuration {
public:
	ExpressFlowMeteringConfiguration(xmlDocPtr doc, xmlNodePtr start);
	~ExpressFlowMeteringConfiguration();

	virtual void configure();
	virtual void setUp();
	virtual void connect(Configuration*);
	virtual void startSystem();
	virtual void stopSystem();
	IpfixAggregator* getIpfixExpressAggregator() { return ipfixAggregator; }

protected:
	Rule* readRule(xmlNodePtr i);
	IpfixAggregator* ipfixAggregator;
	bool running; /**< true between calls to startSystem() and stopSystem() */

	friend class MeteringConfiguration;
};

#endif 	    /* !EXPRESSFLOWMETERING_CONFIGURATION_H_ */
