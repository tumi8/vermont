/*
 released under GPL v2
 (C) by Lothar Braun <mail@lobraun.de>
*/


#ifndef _COLLECTOR_CONFIGURATION_H_
#define _COLLECTOR_CONFIGURATION_H_


#include "ipfix_configuration.h"


#include <concentrator/IpfixCollector.hpp>


#include <vector>


class CollectorConfiguration : public Configuration{
public:
	CollectorConfiguration(xmlDocPtr document, xmlNodePtr startPoint);
	~CollectorConfiguration();

	virtual void configure();
	virtual void connect(Configuration*);
	virtual void startSystem();
	virtual void stopSystem();
	
protected:
	void setUp();
	void readListener(xmlNodePtr i);

	bool running; /**< true between calls to startSystem() and stopSystem() */
	
private:
	struct Listener {
		std::string ipAddress;
		unsigned protocolType;
		uint16_t port;
	};

	std::vector<Listener*> listeners;
	uint16_t observationDomainId;
	
	IpfixCollector* ipfixCollector;
	IpfixParser* ipfixParser;
};

#endif
