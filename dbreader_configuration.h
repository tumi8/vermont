/*
 released under GPL v2
 (C) by Lothar Braun <mail@lobraun.de>
*/

#ifdef DB_SUPPORT_ENABLED

#ifndef _DBREADER_CONFIGURATION_H_
#define _DBREADER_CONFIGURATION_H_


#include "ipfix_configuration.h"


#include <concentrator/IpfixDbReader.hpp>


class DbReaderConfiguration : public Configuration {
public:
	DbReaderConfiguration(xmlDocPtr document, xmlNodePtr startPoint);
	~DbReaderConfiguration();

	virtual void configure();
	virtual void connect(Configuration*);
	virtual void startSystem();
	virtual void stopSystem();

protected:
	void setUp();

	bool running; /**< true between calls to startSystem() and stopSystem() */

private:
	IpfixDbReader* ipfixDbReader;
	std::string hostName;
	std::string dbName;
	std::string userName;
	std::string password;
	unsigned int portNumber;
	uint16_t observationDomainId;
};

#endif

#endif
