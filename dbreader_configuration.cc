/*
 released under GPL v2
 (C) by Lothar Braun <mail@lobraun.de>
*/

#ifdef DB_SUPPORT_ENABLED

#include "dbreader_configuration.h"
#include "exporter_configuration.h"
#include "metering_configuration.h"
#include "flowmetering_configuration.h"
#include "expressflowmetering_configuration.h"
#include "msg.h"


DbReaderConfiguration::DbReaderConfiguration(xmlDocPtr document, xmlNodePtr startPoint)
	: Configuration(document, startPoint), running(false), ipfixDbReader(0), portNumber(0), observationDomainId(0)
{
	xmlChar* idString = xmlGetProp(startPoint, (const xmlChar*)"id");
	if (NULL == idString) {
		THROWEXCEPTION("Got dbreader without unique id!");
	}
	id = configTypes::dbreader + (const char*)idString;
	xmlFree(idString);
}

DbReaderConfiguration::~DbReaderConfiguration()
{
	if (ipfixDbReader) {
		stopSystem();
		delete ipfixDbReader;
	}
}

void DbReaderConfiguration::configure()
{
	msg(MSG_INFO, "DbReaderConfiguration: Start reading dbreader section");
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
		} else if (tagMatches(i, "observationDomainId")) {
			observationDomainId = atoi(getContent(i).c_str());
		} else if (tagMatches(i, "next")) {
			fillNextVector(i);
		}
		i = i->next;
	}
	msg(MSG_INFO, "DbReaderConfiguration: Successfully parsed dbreader section");
	setUp();
}

void DbReaderConfiguration::setUp()
{
	ipfixDbReader = new IpfixDbReader(hostName.c_str(), dbName.c_str(),
					    userName.c_str(), password.c_str(),
					    portNumber, observationDomainId);
	if (!ipfixDbReader) {
		THROWEXCEPTION("DbReaderConfiguration: Could not create IpfixDbReader!");
	}
}

void DbReaderConfiguration::connect(Configuration* c)
{
	ExporterConfiguration* exporter = dynamic_cast<ExporterConfiguration*>(c);
	if (exporter) {
		exporter->createIpfixSender(ipfixDbReader->srcId->observationDomainId);
		IpfixSender* ipfixSender = exporter->getIpfixSender();
		msg(MSG_INFO, "DbReaderConfiguration: Adding ipfixSender-callbacks to dbReader");
		ipfixDbReader->addFlowSink(ipfixSender);
		msg(MSG_INFO, "DbReaderConfiguration: Successfully set up connection between dbReader and Exporter");
		return;
	}
	MeteringConfiguration* metering = dynamic_cast<MeteringConfiguration*>(c);
	if (metering) {
		FlowMeteringConfiguration* fm = metering->getFlowMeteringConfiguration();
		if (!fm) {
			THROWEXCEPTION("DBReaderConfiguration: Cannot connect to an metering process that does not do flowmetering");
		}
		msg(MSG_INFO, "DBReaderConfiguration: Adding dbreader-callbacks to aggregator");
		IpfixAggregator* aggregator = fm->getIpfixAggregator();
		ipfixDbReader->addFlowSink(aggregator);
		msg(MSG_INFO, "DbReaderConfiguration: Successfully set up connection between dbReader and metering process");
		

		return;
	}

	THROWEXCEPTION("Cannot connect %s to dbReader!", c->getId().c_str());
}

void DbReaderConfiguration::startSystem()
{
	if (running) return;
	msg(MSG_INFO, "DbReaderConfiguration: Starting dbReader...");
	ipfixDbReader->start();
	msg(MSG_INFO, "DbReaderConfiguration: Successfully started dbReader");
	running = true;
}

void DbReaderConfiguration::stopSystem()
{
	if (!running) return;
	msg(MSG_INFO, "DbReaderConfiguration: Stopping dbReader...");
	ipfixDbReader->stop();
	msg(MSG_INFO, "DbReaderConfiguration: Successfully stopped dbReader");
	running = false;
}

#endif
