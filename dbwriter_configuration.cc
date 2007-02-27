/*
  released under GPL v2
  (C) Lothar Braun <mail@lobraun.de>
*/

#ifdef DB_SUPPORT_ENABLED

#include "dbwriter_configuration.h"
#include "msg.h"


DbWriterConfiguration::DbWriterConfiguration(xmlDocPtr document, xmlNodePtr startPoint)
	: Configuration(document, startPoint), dbWriter(NULL), portNumber(0), observationDomainId(0), bufferRecords(10)
{
	xmlChar* idString = xmlGetProp(startPoint, (const xmlChar*)"id");
	if (NULL == idString) {
		throw std::runtime_error("Got dbwriter without unique id!");
	}
	id = configTypes::dbwriter + (const char*)idString;
	xmlFree(idString);
}

DbWriterConfiguration::~DbWriterConfiguration()
{
	if (dbWriter) {
		stopIpfixDbWriter(dbWriter);
		destroyIpfixDbWriter(dbWriter);
		deinitializeIpfixDbWriters();
	}
}

void DbWriterConfiguration::setObservationDomainId(uint16_t observationDomainId)
{
        this->observationDomainId = observationDomainId;
        if (dbWriter) {
                dbWriter->srcId.observationDomainId = observationDomainId;
        }
}

void DbWriterConfiguration::configure()
{
	msg(MSG_INFO, "DbWriterConfiguration: Start reading dbwriter section");
	xmlNodePtr i = start->xmlChildrenNode;
	while (NULL != i) {
		if (tagMatches(i, "hostName")) {
			hostName = getContent(i);
		} else if (tagMatches(i, "userName")) {
			userName = getContent(i);
		} else if (tagMatches(i, "dbName")) {
			dbName = getContent(i);
		} else if (tagMatches(i, "password")) {
			password = getContent(i);
		} else if (tagMatches(i, "port")) {
			portNumber = atoi(getContent(i).c_str());
		} else if (tagMatches(i, "bufferRecords")) {
			bufferRecords = atoi(getContent(i).c_str());
		}
		i = i->next;
	}
        setUp();
	msg(MSG_INFO, "DbWriterConfiguration: Successfully parsed dbwriter section");
}

void DbWriterConfiguration::setUp()
{
	initializeIpfixDbWriters();

        if (dbName == "") {
                throw std::runtime_error("DBWriterConfigurations: No database name given!");
        }

	dbWriter = createIpfixDbWriter(hostName.c_str(), dbName.c_str(),
				       userName.c_str(), password.c_str(),
				       portNumber, observationDomainId, bufferRecords);
	if (!dbWriter) {
		throw std::runtime_error("DbWriterConfiguration: Could not create IpfixDbWriter");
	}
}

void DbWriterConfiguration::connect(Configuration*)
{
	throw std::runtime_error("DbWriter is an end target and cannot be connected to something!");
}

void DbWriterConfiguration::startSystem()
{
	msg(MSG_INFO, "DbWriterConfiguration: Starting dbWriter");
	startIpfixDbWriter(dbWriter);
	msg(MSG_INFO, "DbWriterConfiguration: Successfully started dbWriter");
}

#endif
