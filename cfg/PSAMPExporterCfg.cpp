#include "PacketReportingCfg.h"
#include "PSAMPExporterCfg.h"
#include "PSAMPExporterModule.h"

//#include "common/defs.h"

#include <cassert>

PSAMPExporterCfg::PSAMPExporterCfg(XMLElement* elem) 
	:Cfg(elem), exportDelay(0), reporting(NULL) 
{ 
	if (!elem) {
		msg(MSG_FATAL, "%s:$  no xml element set", __FILE__, __LINE__);
		return;
	}


	
	XMLNode::XMLSet<XMLElement*> set = elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("ipfixPacketRestrictions")) {
			maxPacketSize = (uint16_t)getInt("maxPacketSize", 0);
			try {
				exportDelay = getTimeInUnit("maxExportDelay", mSEC, 0);
			} catch (IllegalEntry ie) { /* ignore if not set */ } 
		} else if (e->matches("udpTemplateManagement")) {
			// use 0 as default values for both if the config entry isn't found 
			templateRefreshTime = getInt("templateRefreshTimeout", 0);
			templateRefreshRate = getInt("templateRefreshRate", 0);
		} else if (e->matches("collector")) {
			collectors.push_back(new CollectorCfg(e));
		} else if (e->matches("packetReporting")) {
			reporting = new PacketReportingCfg(e);
		} else if (e->matches("capture_len")) {
			// ignore it, already handled
		} else {
			THROWEXCEPTION("Illegal PSAMPExporter config entry found");
		}
	}

	if (reporting == NULL)
		THROWEXCEPTION("No packetReporting found in psampExporter config");
	
	//uint16_t capture_len = (uint16_t)getInt(capture_len, PCAP_DEFAULT_CAPTURE_LENGTH);
	uint16_t capture_len = (uint16_t)getInt("capture_len", 128);
	// rough estimation of the maximum record length including variable length fields
	recordLength = reporting->getRecordLength() + reporting->getRecordsVariableLen() * capture_len;
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

PSAMPExporterModule* PSAMPExporterCfg::getInstance()
{
	if (!instance) {
		instance = new PSAMPExporterModule(reporting->getTemplate(), 0); // FIXME: 0 is incorrect

		if(recordLength || maxPacketSize)
		{
			// IPFIX packet header: 16 bytes, set header: 4 bytes
			int recordsPerPacket = (maxPacketSize - 16 - 4) / recordLength;
			if(recordsPerPacket <= 0)
				recordsPerPacket = 1;
			msg(MSG_INFO, "Set maximum records per packet to %d", recordsPerPacket);
			instance->setMaxRecords(recordsPerPacket);
		}
		if(exportDelay)
		{
		    msg(MSG_INFO, "Set maximum export timeout to %d", exportDelay);
		    instance->setExportTimeout(exportDelay);
		}
		if(templateRefreshTime || templateRefreshRate)
		{
		    msg(MSG_ERROR, "Exporter: Configuration of templateRefreshRate/Time not yet supported.");
		}
		for (unsigned i = 0; i != collectors.size(); ++i) {
			msg(MSG_DEBUG, "Exporter: adding collector %s:%d to ExporterSink",
					collectors[i]->ipAddress.c_str(),
					collectors[i]->port);
			instance->addCollector(collectors[i]->ipAddress.c_str(),
					collectors[i]->port,
					collectors[i]->protocolType.c_str());
		}

	}
	return instance;
}

bool PSAMPExporterCfg::deriveFrom(Cfg* old)
{
	PSAMPExporterCfg* cfg = dynamic_cast<PSAMPExporterCfg*>(old);
	if (cfg)
		return deriveFrom(cfg);

	THROWEXCEPTION("Can't derive from %s", getName().c_str());
	return false;
}

bool PSAMPExporterCfg::deriveFrom(PSAMPExporterCfg* old)
{
	return false; // FIXME: implement
}

bool CollectorCfg::deriveFrom(Cfg* old)
{
	CollectorCfg* cfg = dynamic_cast<CollectorCfg*>(old);
	if (cfg)
		return deriveFrom(cfg);

	THROWEXCEPTION("Can't derive from %s", getName().c_str());
	return false;
}

bool CollectorCfg::deriveFrom(CollectorCfg* old)
{
	if (ipAddress != old->ipAddress)
		return false;
	if (protocolType != old->protocolType)
		return false;
	if (port != old->port)
		return false;
	
	return true;
}
