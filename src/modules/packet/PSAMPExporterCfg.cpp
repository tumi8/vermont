/*
 * Vermont Configuration Subsystem
 * Copyright (C) 2009 Vermont Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "modules/packet/PacketReportingCfg.h"
#include "modules/packet/PSAMPExporterCfg.h"
#include "modules/packet/PSAMPExporterModule.h"

#include "common/defs.h"

#include <cassert>

PSAMPExporterCfg::PSAMPExporterCfg(XMLElement* elem) 
	: CfgHelper<PSAMPExporterModule, PSAMPExporterCfg>(elem, "psampExporter"),
	templateRefreshTime(0), /* templateRefreshRate(0), */
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
			/* templateRefreshRate = getInt("templateRefreshRate", IS_DEFAULT_TEMPLATE_RECORDINTERVAL, e); */ /* TODO */
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
	if (templateRefreshTime /* || templateRefreshRate */) {
		msg(MSG_DIALOG, "Exporter: Configuration of templateRefreshRate/Time not yet supported.");
	}
	for (unsigned i = 0; i != collectors.size(); ++i) {
		msg(MSG_DEBUG, "PsampExporter: adding collector %s://%s:%d",
				collectors[i]->getProtocol()==132?"SCTP":"UDP",
				collectors[i]->getIpAddress().c_str(),
				collectors[i]->getPort());
		instance->addCollector(collectors[i]->getIpAddress().c_str(),
				collectors[i]->getPort(),
				collectors[i]->getProtocol());
	}

	return instance;
}

bool PSAMPExporterCfg::deriveFrom(PSAMPExporterCfg* old)
{
	return false; // FIXME: implement
}
