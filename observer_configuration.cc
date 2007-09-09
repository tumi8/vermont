/*
 released under GPL v2
 (C) by Lothar Braun <mail@lobraun.de>
*/


#include "observer_configuration.h"
#include "metering_configuration.h"
#include "packetselection_configuration.h"
#include "flowmetering_configuration.h"
#include "common/msg.h"

#include <sampler/Template.h>
#include <sampler/PacketProcessor.h>
#include <sampler/Filter.h>
#include <sampler/ExporterSink.h>


#include <stdexcept>
#include <cstdlib>


ObserverConfiguration::ObserverConfiguration(xmlDocPtr document, xmlNodePtr startPoint)
	: Configuration(document, startPoint), observationDomain(0), offlinemode(false), replaceOfflineTimestamps(false),
       	offlineSpeed(1.0), captureLength(0), observer(NULL)
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
	msg(MSG_INFO, "ObserverConfiguration: Start reading observationPoint section");
	xmlNodePtr i = start->xmlChildrenNode;
	while (NULL != i) {
		if (tagMatches(i, "observationDomainId")) {
			observationDomain = std::atoi(getContent(i).c_str());
		} else if (tagMatches(i, "type")) {
			type = getContent(i);
			if (type != "pcap") {
				msg(MSG_FATAL, "ObserverConfiguration: Vermont does not provide any observer type but pcap");
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
	msg(MSG_INFO, "ObserverConfiguration: Successfully parsed observationPoint section");
}

void ObserverConfiguration::parseParameters(xmlNodePtr p)
{
	bool interfaceExists = false;
	xmlNodePtr i = p->xmlChildrenNode;
	while (NULL != i) {
		if (tagMatches(i, "interface") && !offlinemode) {
			interfaceExists = true;
			interface = getContent(i);
		} else if (tagMatches(i, "pcap_filter")) {
			pcapFilter = getContent(i);
		} else if (tagMatches(i, "capture_len")) {
			captureLength = atoi(getContent(i).c_str());
		} else if (tagMatches(i, "filename")) {
			interface = getContent(i);
			offlinemode = true;
		} else if (tagMatches(i, "replace_timestamps") && (getContent(i) != "false")) {
			replaceOfflineTimestamps = true;
		} else if (tagMatches(i, "speed_multiplier")) {
			offlineSpeed = (float)(atof(getContent(i).c_str()));
		}
		i = i->next;
	}
	if (interfaceExists && offlinemode)
	    msg(MSG_INFO, "ObserverConfiguration: interface and filename in observationPoint/parameter section. Interface is ignored.");
	if (replaceOfflineTimestamps && !offlinemode)
	    msg(MSG_INFO, "ObserverConfiguration: replace_timestamps without filename in observationPoint/parameter is ignored.");
	if ((offlineSpeed != 1.0) && !offlinemode)
	    msg(MSG_INFO, "ObserverConfiguration: speed_multiplier without filename in observationPoint/parameter is ignored.");
}


void ObserverConfiguration::setUp()
{
	observer = new Observer(interface.c_str(), &packetManager, offlinemode);
	if (captureLength) {
		if (!observer->setCaptureLen(captureLength)) {
			msg(MSG_ERROR, "ObserverConfiguration: wrong snaplen specified - using %d", observer->getCaptureLen());
		}
	}
	else
	{
	    // we need capture length in connect()
	    captureLength = observer->getCaptureLen();
	}
	if (offlinemode) {
	    if(replaceOfflineTimestamps)
		observer->replaceOfflineTimestamps();
	    if(offlineSpeed != 1.0)
		observer->setOfflineSpeed(offlineSpeed);
	}
	
	//pcapChar = new char[pcapFilter.size() + 1];
	//strncpy(pcapChar, pcapFilter.c_str(), pcapFilter.size() + 1);	
	if (!observer->prepare(pcapFilter.c_str())) {
		msg(MSG_FATAL, "ObserverConfiguration: preparing failed");
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
		msg(MSG_DEBUG, "ObserverConfiguration: Connecting observer to metering process");
		metering->setObservationDomainId(observationDomain);
		metering->setCaptureLength(captureLength);
		PacketSelectionConfiguration* ps = metering->getPacketSelectionConfiguration();
		observer->addReceiver(ps->getFilters());
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
	observer->terminateCapture();
}

