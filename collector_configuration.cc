/*
 released under GPL v2
 (C) by Lothar Braun <mail@lobraun.de>
*/


#include "collector_configuration.h"
#include "metering_configuration.h"
#include "flowmetering_configuration.h"
#include "expressflowmetering_configuration.h"
#include "exporter_configuration.h"
#include "dbwriter_configuration.h"

#include "concentrator/IpfixAggregator.hpp"
#include "concentrator/IpfixReceiverUdpIpV4.hpp"

#include "common/msg.h"


CollectorConfiguration::CollectorConfiguration(xmlDocPtr document, xmlNodePtr startPoint)
	: Configuration(document, startPoint), running(false), observationDomainId(0),
		ipfixCollector(0), ipfixParser(0)
{
	xmlChar* idString = xmlGetProp(startPoint, (const xmlChar*)"id");
	if (NULL == idString) {
		THROWEXCEPTION("Got collector without unique id!");
	}
	id = configTypes::collector + (const char*)idString;
	xmlFree(idString);
}

CollectorConfiguration::~CollectorConfiguration()
{
	for (unsigned i = 0; i != listeners.size(); ++i) {
		delete listeners[i];
	}
	if (ipfixCollector) {
		stopSystem();
		delete ipfixCollector;
	}
}

void CollectorConfiguration::configure()
{
	msg(MSG_INFO, "CollectorConfiguration: Start reading packetReporting section");
	xmlNodePtr i = start->xmlChildrenNode;
	while (NULL != i) {
		if (tagMatches(i, "listener")) {
			readListener(i);
		} else if (tagMatches(i, "udpTemplateLifetime")) {
			msg(MSG_DEBUG, "Don't know how to handle udpTemplateLifetime! Ignored.");
		} else if (tagMatches(i, "observationDomainId")) {
			observationDomainId = atoi(getContent(i).c_str());
		} else if (tagMatches(i, "next")) {
			fillNextVector(i);
		}
		i = i->next;
	}
	setUp();
	msg(MSG_INFO, "CollectorConfiguration: Successfully parsed collectingProcess section");
}

void CollectorConfiguration::readListener(xmlNodePtr p)
{
	xmlNodePtr i = p->xmlChildrenNode;

	Listener* listener = new Listener();
	listener->port = 4739; // standard port for IPFIX

	while (NULL != i) {
		if (tagMatches(i, "ipAddressType")) {
			// we only have ipv4 at the moment
			// so nothing is implemented yet for ipv6
			if (getContent(i) != "4") {
				msg(MSG_ERROR, "Only ipv4 is supported at the moment. \"ipAddressType\" will be ignored at the moment");
			}
		} else  if (tagMatches(i, "ipAddress")) {
			listener->ipAddress = getContent(i);
			msg(MSG_DEBUG, "Listening on a specific interface isn't supported right now. Vermont will listen on all interfaces. \"ipAddress\" will be ignored at the moment");
		} else if (tagMatches(i, "transportProtocol")) {
			listener->protocolType = atoi(getContent(i).c_str());
			if (listener->protocolType != 17) {
				msg(MSG_ERROR, "Vermont doesn't support any protocol other than UDP (17). transportProtocol will be ignored at the moment");
			}
		} else if (tagMatches(i, "port")) {
			listener->port = (uint16_t)atoi(getContent(i).c_str());
		}
		i = i->next;
	}
	listeners.push_back(listener);
}

void CollectorConfiguration::setUp()
{
	ipfixCollector = new IpfixCollector;
	if (!ipfixCollector) {
		THROWEXCEPTION("Could not create collector");
	}

	for (unsigned i = 0; i != listeners.size(); ++i) {
		IpfixReceiver* ipfixReceiver = new IpfixReceiverUdpIpV4(listeners[i]->port);
		if (!ipfixReceiver) {
			THROWEXCEPTION("Could not create receiver");
		}
		ipfixCollector->addIpfixReceiver(ipfixReceiver);
	}

	ipfixParser = new IpfixParser;
	if (!ipfixParser) {
		THROWEXCEPTION("Could not create IPFIX parser");
	}
}

void CollectorConfiguration::connect(Configuration* c)
{
	// the collector can put it's data only into
	// - an metering process which is aggregating or
	// - an exporting process
	

	MeteringConfiguration* metering = dynamic_cast<MeteringConfiguration*>(c);
	if (metering) {
		metering->setObservationDomainId(observationDomainId);
		FlowMeteringConfiguration* fm = metering->getFlowMeteringConfiguration();

		if (!fm) {
			THROWEXCEPTION("Metering process isn't aggregating ->"
						 " cannot connect it to an collector!");
		}


		msg(MSG_DEBUG, "CollectorConfiguration: Got metering process which is aggreagting");
		IpfixAggregator* aggregator = fm->getIpfixAggregator();
		if (!aggregator) {
			THROWEXCEPTION("CollectorConfiguration: ipfixAggregator is null -> This is a bug! Please report it");
			}
		msg(MSG_DEBUG, "Adding aggregator to ipfixParser");
		ipfixParser->addFlowSink(aggregator);
		msg(MSG_DEBUG, "Adding ipfixPacketProcessor to ipfixCollector");
		ipfixCollector->addIpfixPacketProcessor(ipfixParser);
		msg(MSG_DEBUG, "Sucessfully set up connection between collector and aggregator");
		return;
	}
	
	ExporterConfiguration* exporter = dynamic_cast<ExporterConfiguration*>(c);
	if (exporter) {
		exporter->createIpfixSender(observationDomainId);
		IpfixSender* ipfixSender = exporter->getIpfixSender();
		msg(MSG_DEBUG, "Adding IpfixSender callbacks to IpfixParser");
		ipfixParser->addFlowSink(ipfixSender);
		msg(MSG_DEBUG, "Adding IpfixPacketProcessor to IpfixCollector");
		ipfixCollector->addIpfixPacketProcessor(ipfixParser);
		msg(MSG_DEBUG, "Successfully set up connection between collector and exporter");
		return;
	}

#ifdef DB_SUPPORT_ENABLED
	DbWriterConfiguration* dbWriterConfiguration = dynamic_cast<DbWriterConfiguration*>(c);
	if (dbWriterConfiguration) {
		msg(MSG_DEBUG, "Adding DBwriter to IpfixCollector");
                dbWriterConfiguration->setObservationDomainId(observationDomainId);
		ipfixParser->addFlowSink(dbWriterConfiguration->getDbWriter());
		msg(MSG_DEBUG, "Adding IpfixPacketProcessor to IpfixCollector");
		ipfixCollector->addIpfixPacketProcessor(ipfixParser);
		msg(MSG_DEBUG, "Successfully set up connction between collector and dbwriter");
		return;
	}
#endif

	THROWEXCEPTION("Cannot connect %s to a collector!", c->getId().c_str());
}

void CollectorConfiguration::startSystem()
{
	if (running) return;
	msg(MSG_DEBUG, "CollectorConfiguration: Starting collecting process");
	ipfixCollector->start();
	running = true;
}

void CollectorConfiguration::stopSystem()
{
	if (!running) return;
	msg(MSG_DEBUG, "CollectorConfiguration: Stopping collecting process");
	ipfixCollector->stop();
	running = false;
}

