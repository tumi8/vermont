#include "exporter_configuration.h"
#include "metering_configuration.h"

#include <sampler/ExporterSink.h>

ExporterConfiguration::ExporterConfiguration(xmlDocPtr document, xmlNodePtr startPoint)
	: Configuration(document, startPoint), exporterSink(0), ipfixSender(0)
{
	xmlChar* idString = xmlGetProp(startPoint, (const xmlChar*)"id");
	if (NULL == idString) {
		throw std::runtime_error("Got exporter without unique id!");
	}
	id = configTypes::exporter + (const char*)idString;
	xmlFree(idString);
}

ExporterConfiguration::~ExporterConfiguration()
{
	for (unsigned i = 0; i != collectors.size(); ++i) {
		delete collectors[i];
	}
	delete exporterSink;
	if (ipfixSender) {
		stopIpfixSender(ipfixSender);
		destroyIpfixSender(ipfixSender);
		deinitializeIpfixSenders();
	}
}

void ExporterConfiguration::configure()
{
	msg(MSG_INFO, "ExporterConfiguration: Start reading exportingProcess section");
	xmlNodePtr i = start->xmlChildrenNode;
	while (NULL != i) {
		if (tagMatches(i, "ipfixPacketRestrictions")) {
			readPacketRestrictions(i);
		} else if (tagMatches(i, "udpTemplateManagement")) {
			readUdpTemplateManagement(i);
		} else if (tagMatches(i, "collector")) {
			readCollector(i);
		}
		i = i->next;
	}
	msg(MSG_INFO, "ExporterConfiguration: Successfully parsed exportingProcess section");
}

void ExporterConfiguration::readPacketRestrictions(xmlNodePtr p)
{
	xmlNodePtr i = p->xmlChildrenNode;
	while (NULL != i) {
		if (tagMatches(i, "maxPacketSize")) {
			maxPacketSize = (uint16_t)atoi(getContent(i).c_str());
		} else if (tagMatches(i, "maxExportDelay")) {
			exportDelay = getTimeInMsecs(i);
		}
		i = i->next;
	}
}

void ExporterConfiguration::readUdpTemplateManagement(xmlNodePtr p)
{
	xmlNodePtr i = p->xmlChildrenNode;
	while (NULL != i) {
		if (tagMatches(i, "templateRefreshTimeout")) {
			templateRefreshTime = getTimeInMsecs(i);
		} else if (tagMatches(i, "templateRefreshRate")) {
			templateRefreshRate = (unsigned)atoi(getContent(i).c_str());
		}
		i = i->next;
	}
}

void ExporterConfiguration::readCollector(xmlNodePtr p)
{
	xmlNodePtr i = p->xmlChildrenNode;

	Collector* c = new Collector();
	
	while (NULL != i) {
		if (tagMatches(i, "ipAddressType")) {
			// we only have ipv4 at the moment
			// so nothing is implemented yet for ipv6
			c->ipAddressType = 4;
		} else  if (tagMatches(i, "ipAddress")) {
			c->ipAddress = getContent(i);
		} else if (tagMatches(i, "transportProtocol")) {
			c->protocolType = getContent(i);
			if (c->protocolType == "17") {
				c->protocolType = "UDP";
			}
		} else if (tagMatches(i, "port")) {
			c->port = (uint16_t)atoi(getContent(i).c_str());
		}		
		i = i->next;
	}
	collectors.push_back(c);
}

void ExporterConfiguration::setUp()
{

}

void ExporterConfiguration::createExporterSink(Template* t, uint16_t sourceId)
{
	msg(MSG_INFO, "Creating exporter sink");
	exporterSink = new ExporterSink(t, sourceId);
	for (unsigned i = 0; i != collectors.size(); ++i) {
		msg(MSG_DEBUG, "Exporter: adding collector %s:%d to ExporterSink",
		    collectors[i]->ipAddress.c_str(),
		    collectors[i]->port);
		exporterSink->addCollector(collectors[i]->ipAddress.c_str(),
					   collectors[i]->port,
					   collectors[i]->protocolType.c_str());
	}
}

void ExporterConfiguration::createIpfixSender(uint16_t sourceId)
{
	if (collectors.empty()) {
		msg(MSG_INFO, "Aggreagtor won't export it's result to any collector");
	}

	initializeIpfixSenders();
	msg(MSG_DEBUG, "Exporter: Creating IpfixSender");
	ipfixSender = ::createIpfixSender(sourceId,
					  collectors[0]->ipAddress.c_str(),
					  collectors[0]->port);
	if (!ipfixSender) {
		throw std::runtime_error("Could not create IpfixSender!");
	}

	for (unsigned i = 1; i != collectors.size(); ++i) {
		if (ipfixSenderAddCollector(ipfixSender,
					    collectors[i]->ipAddress.c_str(),
					    collectors[i]->port)) {
			msg(MSG_ERROR, "Config: error adding collector %s:%d to IpfixSender",
			    collectors[i]->ipAddress.c_str(), collectors[i]->port);
		}
	}
	// we need to start IpfixSender right here, because ipfixAggregator
	// needs a running IpfixSender before it can be created
	// TODO: FIX THIS!
	startIpfixSender(ipfixSender);
}

void ExporterConfiguration::connect(Configuration*)
{
	throw std::runtime_error("Exporter is an end target and cannot be connected to something!");
}

void ExporterConfiguration::startSystem()
{
	if (exporterSink) {
		msg(MSG_DEBUG, "ExporterConfiguration: Starting ExporterSink for Sampler");
		exporterSink->runSink();
	} else if (ipfixSender) {
		msg(MSG_DEBUG, "ExporterConfiguration: Running IpfixSenders.");
		// ipfixSender already runs (see createIpfixSender())
	} else {
		throw std::runtime_error("Can neither start an ExporterSink, nor an IpfixSender -> something is broken!");
	}
}
