/*
 released under GPL v2
 (C) by Lothar Braun <mail@lobraun.de>
*/


#include "observer_configuration.h"
#include "metering_configuration.h"
#include "packetselection_configuration.h"
#include "flowmetering_configuration.h"
#include "common/msg.h"
#include "common/PacketInstanceManager.h"

#include <reconf/FilterModule.h>

#include <sampler/Template.h>
#include <sampler/PacketProcessor.h>
#include <sampler/ExporterSink.h>


#include <stdexcept>
#include <cstdlib>


ObserverConfiguration::ObserverConfiguration(xmlDocPtr document, xmlNodePtr startPoint)
	: Configuration(document, startPoint), observationDomain(0), captureLength(0), observer(NULL)
{
	xmlChar* idString = xmlGetProp(startPoint, (const xmlChar*)"id");
	if (NULL == idString) {
		THROWEXCEPTION("Got observer without unique id!");
	}
	id = configTypes::observer + (const char*)idString;
	xmlFree(idString);
}

ObserverConfiguration::~ObserverConfiguration()
{
	delete observer;
}


void ObserverConfiguration::configure()
{
	msg(MSG_INFO, "ObserverConfiguration: Start reading observationProcess section");
	xmlNodePtr i = start->xmlChildrenNode;
	while (NULL != i) {
		if (tagMatches(i, "observationDomainId")) {
			observationDomain = std::atoi(getContent(i).c_str());
		} else if (tagMatches(i, "type")) {
			type = getContent(i);
			if (type != "pcap") {
				msg(MSG_FATAL, "Vermont does not provide any observer type but pcap");
				THROWEXCEPTION("Could not read observer configuration");
			}
		} else if (tagMatches(i, "parameters")) {
			parseParameters(i);
		} else if (tagMatches(i, "next")) {
			fillNextVector(i);
		}
		i = i->next;
	}

	setUp();
	msg(MSG_INFO, "ObserverConfiguration: Successfully parsed observationProcess section");
}

void ObserverConfiguration::parseParameters(xmlNodePtr p)
{
	xmlNodePtr i = p->xmlChildrenNode;
	while (NULL != i) {
		if (tagMatches(i, "interface")) {
			interface = getContent(i);
		} else if (tagMatches(i, "pcap_filter")) {
			pcapFilter = getContent(i);
		} else if (tagMatches(i, "capture_len")) {
			captureLength = atoi(getContent(i).c_str());
		}
		i = i->next;
	}
}


void ObserverConfiguration::setUp()
{
	observer = new Observer(interface.c_str(), PacketInstanceManager::getManager());
	if (captureLength) {
		if (!observer->setCaptureLen(captureLength)) {
			msg(MSG_FATAL, "Observer: wrong snaplen specified - using %d", observer->getCaptureLen());
		}
	}
	else
	{
	    // we need capture length in connect()
	    captureLength = observer->getCaptureLen();
	}
	
	
	//pcapChar = new char[pcapFilter.size() + 1];
	//strncpy(pcapChar, pcapFilter.c_str(), pcapFilter.size() + 1);	
	if (!observer->prepare(pcapFilter.c_str())) {
		msg(MSG_FATAL, "Observer: preparing failed");
		THROWEXCEPTION("Observer setup failed!");
	}
}


void ObserverConfiguration::connect(Configuration* c)
{
	// the observer process can put it's data into
	// - a metering process that does PacketSelection
	//   (since every metering process _must_ perform
	//   some sort of packet selection (even if it is "take all")
	//   every metering process can be connected to an observer

	MeteringConfiguration* metering = dynamic_cast<MeteringConfiguration*>(c);
	if (metering) {
		msg(MSG_DEBUG, "Connecting observer to metering process");
		metering->setObservationDomainId(observationDomain);
		metering->setCaptureLength(captureLength);
		PacketSelectionConfiguration* ps = metering->getPacketSelectionConfiguration();
		observer->connectTo(ps->getFilters());
		return;
	}
	
	THROWEXCEPTION("Cannot connect Observer to %s!", c->getId().c_str());
}

void ObserverConfiguration::startSystem()
{
	if (!observer) {
		DPRINTF("stopping non-initialized Observer");
		return;
	}
	observer->startCapture();
}

void ObserverConfiguration::stopSystem()
{
	if (!observer) {
		DPRINTF("stopping non-initialized Observer");
		return;
	}
	observer->shutdown();
}

