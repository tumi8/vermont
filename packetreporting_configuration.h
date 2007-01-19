/*
 released under GPL v2
 (C) by Lothar Braun <mail@lobraun.de>
*/


#ifndef   	PACKETREPORTING_CONFIGURATION_H_
#define   	PACKETREPORTING_CONFIGURATION_H_

#include "ipfix_configuration.h"

class Template;
class MeteringConfiguration;

class PacketReportingConfiguration : public Configuration {
public:
	PacketReportingConfiguration(xmlDocPtr doc, xmlNodePtr start);
	~PacketReportingConfiguration();

	virtual void configure();
	virtual void setUp();
	virtual void connect(Configuration*);
	virtual void startSystem();

protected:
	std::vector<ReportedIE*> exportedFields;
	int templateId;
	Template* t;
	uint16_t observationDomainId;
	uint16_t recordLength;
	uint16_t recordVLFields;

	friend class MeteringConfiguration;
};

#endif 	    /* !PACKETREPORTING_CONFIGURATION_H_ */
