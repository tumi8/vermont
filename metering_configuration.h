/*
 released under GPL v2
 (C) by Lothar Braun <mail@lobraun.de>
*/


#ifndef _METERING_CONFIGURATION_H_
#define _METERING_CONFIGURATION_H_


#include "ipfix_configuration.h"

#include <concentrator/IpfixAggregator.hpp>

#include <vector>
#include <ctime>


class FlowMeteringConfiguration;
//class ExpressFlowMeteringConfiguration;
class PacketSelectionConfiguration;
class PacketReportingConfiguration;

class MeteringConfiguration : public Configuration {
public:
	MeteringConfiguration(xmlDocPtr document, xmlNodePtr startPoint);
	~MeteringConfiguration();
	
	virtual void configure();
	virtual void connect(Configuration*);
	virtual void startSystem();
	virtual void stopSystem();

	void setObservationDomainId(uint16_t id);
	void setCaptureLength(int len);
	void setDataLinkType(int type);

	FlowMeteringConfiguration* getFlowMeteringConfiguration() { return flowMetering; }
	FlowMeteringConfiguration* getExpressFlowMeteringConfiguration() { return expressflowMetering; }
	PacketReportingConfiguration* getPacketReportingConfiguration() { return packetReporting; }
	PacketSelectionConfiguration* getPacketSelectionConfiguration();
private:
	PacketSelectionConfiguration* packetSelection;
	PacketReportingConfiguration* packetReporting;
	FlowMeteringConfiguration* flowMetering;
	FlowMeteringConfiguration* expressflowMetering;

	uint16_t observationDomainId;
	int captureLength;
	int dataLinkType;
};

#endif
