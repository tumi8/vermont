/*
  released under GPL v2
  (C) Lothar Braun <mail@lobraun.de>
*/

#ifdef DB_SUPPORT_ENABLED

#ifndef _DBWRITER_CONFIGURATION_H_
#define _DBWRITER_CONFIGURATION_H_


#include "ipfix_configuration.h"


#include <concentrator/IpfixDbWriter.h>


#include <string>



class DbWriterConfiguration : public Configuration {
public:
	DbWriterConfiguration(xmlDocPtr document, xmlNodePtr startPoint);
	~DbWriterConfiguration();

	virtual void configure();
	virtual void connect(Configuration*);
	virtual void startSystem();

        void setObservationDomainId(uint16_t observationDomainId);

	IpfixDbWriter* getDbWriter() { return dbWriter; }

protected:
	void setUp();

private:
	IpfixDbWriter* dbWriter;
	std::string hostName;
	std::string dbName;
	std::string userName;
	std::string password;
	unsigned int portNumber;
	uint16_t observationDomainId;
	int bufferRecords;
};

#endif

#endif
