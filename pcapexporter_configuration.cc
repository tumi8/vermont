#include "pcapexporter_configuration.h"
#include "metering_configuration.h"

#include <sampler/PcapExporterSink.h>
#include <common/msg.h>

PcapExporterConfiguration::PcapExporterConfiguration(xmlDocPtr document, xmlNodePtr startPoint)
	: Configuration(document, startPoint), exporterSink(0)
{
	xmlChar* idString = xmlGetProp(startPoint, (const xmlChar*)"id");
	if (NULL == idString) {
		THROWEXCEPTION("Got pcapExporter without unique id!");
	}
	id = configTypes::pcapExporter + (const char*)idString;
	xmlFree(idString);
}

PcapExporterConfiguration::~PcapExporterConfiguration()
{
	delete exporterSink;
}

void PcapExporterConfiguration::configure()
{
	msg(MSG_INFO, "PcapExporterConfiguration: Start reading pcapExportingProcess section");
	xmlNodePtr i = start->xmlChildrenNode;
	while (NULL != i) {
		if (tagMatches(i, "fileName")) {
			fileName = getContent(i);
		}
		i = i->next;
	}
	setUp();
	msg(MSG_INFO, "PcapExporterConfiguration: Successfully parsed PcapExportingProcess section");
}

void PcapExporterConfiguration::setUp()
{
	exporterSink = new PcapExporterSink(fileName);
}

void PcapExporterConfiguration::connect(Configuration*)
{
	THROWEXCEPTION("PcapExporter is an end target and cannot be connected to something!");
}

void PcapExporterConfiguration::startSystem()
{
	if (!exporterSink)
		THROWEXCEPTION("Cannot start exporterSink, exporterSink does not exist!");
	exporterSink->runSink();
}

void PcapExporterConfiguration::stopSystem()
{
	if (exporterSink)
		exporterSink->terminateSink();
}

