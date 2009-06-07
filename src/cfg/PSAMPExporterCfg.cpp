#include "PacketReportingCfg.h"
#include "PSAMPExporterCfg.h"
#include "reconf/PSAMPExporterModule.h"

#include "common/defs.h"

#include <cassert>

PSAMPExporterCfg::PSAMPExporterCfg(XMLElement* elem) 
	: CfgHelper<PSAMPExporterModule, PSAMPExporterCfg>(elem, "psampExporter"),
	templateRefreshTime(0), templateRefreshRate(0),
	maxPacketSize(0), exportDelay(0), reporting(NULL) 
{ 
	if (!elem) return;

	observationDomainId = getInt("observationDomainId", 0);

	// determine captureLen
	// FIXME: undocumented parameter, this value should come from observer
	int captureLen = getInt("captureLen", PCAP_DEFAULT_CAPTURE_LENGTH);
	
	XMLNode::XMLSet<XMLElement*> set = elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("ipfixPacketRestrictions")) {
			maxPacketSize = (uint16_t)getInt("maxPacketSize", 0, e);
			exportDelay = getTimeInUnit("maxExportDelay", mSEC, 0, e);
		} else if (e->matches("udpTemplateManagement")) {
			// use 0 as default values for both if the config entry isn't found 
			templateRefreshTime = getTimeInUnit("templateRefreshTimeout", SEC, IS_DEFAULT_TEMPLATE_TIMEINTERVAL, e);
			templateRefreshRate = getInt("templateRefreshRate", IS_DEFAULT_TEMPLATE_RECORDINTERVAL, e);
		} else if (e->matches("collector")) {
			collectors.push_back(new CollectorCfg(e));
		} else if (e->matches("packetReporting")) {
			reporting = new PacketReportingCfg(e);
		} else if (e->matches("captureLen") || e->matches("observationDomainId")) {
			// ignore it, already handled
		} else {
			THROWEXCEPTION("Illegal PSAMPExporter config entry \"%s\"found",
					e->getName().c_str());
		}
	}

	if (reporting == NULL)
		THROWEXCEPTION("No packetReporting found in psampExporter config");
	
	// rough estimation of the maximum record length including variable length fields
	recordLength =  reporting->getRecordLength() + 
			reporting->getRecordsVariableLen() * captureLen;
} 

PSAMPExporterCfg* PSAMPExporterCfg::create(XMLElement* elem)
{
	assert(elem);
	assert(elem->getName() == getName());
	return new PSAMPExporterCfg(elem);
}

PSAMPExporterCfg::~PSAMPExporterCfg()
{
	for (size_t i = 0; i < collectors.size(); i++)
		delete collectors[i];

	delete reporting;
}

PSAMPExporterModule* PSAMPExporterCfg::createInstance()
{
	instance = new PSAMPExporterModule(reporting->getTemplate(), observationDomainId);

	if (recordLength || maxPacketSize) {
		int recordsPerPacket = 1;
		
		printf ("create recordLen = %d\n\n", recordLength);
		printf ("create maxPaSize = %d\n\n", maxPacketSize);
		
		if (recordLength) { 
			// IPFIX packet header: 16 bytes, set header: 4 bytes
			recordsPerPacket = (maxPacketSize - 16 - 4) / recordLength;

			if(recordsPerPacket <= 0)
				recordsPerPacket = 1;
		}

		msg(MSG_INFO, "Set maximum records per packet to %d", recordsPerPacket);
		instance->setMaxRecords(recordsPerPacket);
	}
	if (exportDelay) {
		msg(MSG_INFO, "Set maximum export timeout to %d", exportDelay);
		instance->setExportTimeout(exportDelay);
	}
	if (templateRefreshTime || templateRefreshRate) {	
		msg(MSG_DIALOG, "Exporter: Configuration of templateRefreshRate/Time not yet supported.");
	}
	for (unsigned i = 0; i != collectors.size(); ++i) {
		msg(MSG_DEBUG, "PsampExporter: adding collector %s://%s:%d",
				collectors[i]->getProtocolType()==SCTP?"SCTP":"UDP",
				collectors[i]->getIpAddress().c_str(),
				collectors[i]->getPort());
		instance->addCollector(collectors[i]->getIpAddress().c_str(),
				collectors[i]->getPort(),
				collectors[i]->getProtocolType());
	}

	return instance;
}

bool PSAMPExporterCfg::deriveFrom(PSAMPExporterCfg* old)
{
	return false; // FIXME: implement
}
