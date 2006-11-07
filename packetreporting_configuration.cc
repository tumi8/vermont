/*
 released under GPL v2
 (C) by Lothar Braun <mail@lobraun.de>
*/


#include "packetreporting_configuration.h"
#include "exporter_configuration.h"

#include <sampler/Template.h>

PacketReportingConfiguration::PacketReportingConfiguration(xmlDocPtr doc, xmlNodePtr start)
	: Configuration(doc, start), templateId(0), t(0), observationDomainId(0)
{

}


PacketReportingConfiguration::~PacketReportingConfiguration()
{
	for (unsigned i = 0; i != exportedFields.size(); ++i) {
		delete exportedFields[i];
	}
	delete t;
}

void PacketReportingConfiguration::configure()
{
	msg(MSG_INFO, "PacketReportingConfiguration: Start reading packetReporting section");
	xmlNodePtr i = start->xmlChildrenNode;
	bool gotTemplateId = false;
	while (NULL != i) {
		if (tagMatches(i, "reportedIE")) {
			exportedFields.push_back(new ReportedIE(i, *this));
		}
		if (tagMatches(i, "templateId")) {
			templateId = atoi(getContent(i).c_str());
			gotTemplateId = true;
		}
		i = i->next;
	}

	if (!gotTemplateId) {
		throw std::runtime_error("PacketReportingConfiguration: Got PacketReporting without template ID");
	}

	setUp();
	msg(MSG_INFO, "PacketReportingConfiguration: Successfully parsed packetReporting section");
}

void PacketReportingConfiguration::setUp()
{
	msg(MSG_DEBUG, "Creating template");
	t = new Template(templateId);
	
	for (unsigned i = 0; i != exportedFields.size(); ++i) {
		int tmpId = exportedFields[i]->getId();
		if (!ipfix_id_rangecheck(tmpId)) {
			msg(MSG_ERROR, "Template: ignoring template field %s -> %d - rangecheck not ok", exportedFields[i]->getName().c_str(), tmpId);
			continue;
		}
		
		const ipfix_identifier *id;
		if( (tmpId == -1) || ((id=ipfix_id_lookup(tmpId)) == NULL) ) {
			msg(MSG_ERROR, "Template: ignoring unknown template field %s", exportedFields[i]->getName().c_str());
			continue;
		}
		
		int fieldLength = id->length;
		if (exportedFields[i]->hasOptionalLength()) {
			if (fieldLength == 0) {
				fieldLength = exportedFields[i]->getLength();
			} else {
				msg(MSG_ERROR, "Template: this is not a variable length field, ignoring optional length");
			}
		}
		msg(MSG_INFO, "Template: adding %s -> ID %d with size %d", exportedFields[i]->getName().c_str(), id->id, fieldLength);
		t->addField((uint16_t)id->id, fieldLength);
	}
	msg(MSG_DEBUG, "Template: got %d fields", t->getFieldCount());

}

void PacketReportingConfiguration::connect(Configuration* c)
{
}

void PacketReportingConfiguration::startSystem()
{

}
