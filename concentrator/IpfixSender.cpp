/*
 * IPFIX Concentrator Module Library
 * Copyright (C) 2004 Christoph Sommer <http://www.deltadevelopment.de/users/christoph/ipfix/>
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

#include "IpfixSender.hpp"
#include "ipfix.hpp"

#include "common/msg.h"
#include "common/Time.h"
#include "reconf/Timer.h"

#include <sstream>
#include <stdexcept>
#include <string.h>

using namespace std;

/*
 we start OUR template IDs at <this>
 we need our own template IDs and they should be unique
 */
#define SENDER_TEMPLATE_ID_LOW 10000
/* go back to SENDER_TEMPLATE_ID_LOW if _HI is reached */
#define SENDER_TEMPLATE_ID_HI 60000

/**
 * Creates a new IPFIX Exporter. Do not forget to call @c startIpfixSender() to begin sending
 * @param sourceID Source ID this exporter will report
 * @param ip destination collector's address
 * @param port destination collector's port
 * @return handle to use when calling @c destroyIpfixSender()
 */
IpfixSender::IpfixSender(uint16_t observationDomainId, const char* ip, uint16_t port, ipfix_transport_protocol proto, uint32_t udpRateLimit)
	: statSentPackets(0),
	  noCachedRecords(0),
	  recordCacheTimeout(IS_DEFAULT_RECORDCACHETIMEOUT),
	  timeoutRegistered(false),
	  recordsAlreadySent(false),
	  udpRateLimit(udpRateLimit)
{
	ipfix_exporter** exporterP = &this->ipfixExporter;
	statSentDataRecords = 0;
	statPacketsInFlows = 0;
	currentTemplateId = 0;
	lastTemplateId = SENDER_TEMPLATE_ID_LOW;
	
	nextTimeout.tv_sec = 0;
	nextTimeout.tv_nsec = 0;
	curTimeStep.tv_sec = 0;
	curTimeStep.tv_usec = 0;

	if(ipfix_init_exporter(observationDomainId, exporterP) != 0) {
		msg(MSG_FATAL, "sndIpfix: ipfix_init_exporter failed");
		goto out;
	}

	if (ip && port) {
		addCollector(ip, port, proto);
	}
	
	msg(MSG_DEBUG, "IpfixSender: running");
	return;
	
out:
	THROWEXCEPTION("IpfixSender creation failed");
	return;	
}

/**
 * Removes a collector from the list of Collectors to send Records to
 */
IpfixSender::~IpfixSender()
{
	shutdown(false);
	
	ipfix_exporter* exporter = (ipfix_exporter*)ipfixExporter;
	ipfix_deinit_exporter(exporter);
}

/**
 * Add another IPFIX collector to export the stream to
 * the lowlevel stuff in handled by underlying ipfixlolib
 * @param ips handle to the Exporter
 * @param ip string of the IP
 * @param port port number
 * FIXME: support for other than UDP
 */
void IpfixSender::addCollector(const char *ip, uint16_t port, ipfix_transport_protocol proto)
{
	ipfix_exporter *ex = (ipfix_exporter *)ipfixExporter;

	msg(MSG_INFO, "IpfixSender: adding %s://%s:%d to exporter", (proto==UDP)?"UDP":"SCTP", ip, port);

	if(ipfix_add_collector(ex, ip, port, proto) != 0) {
		msg(MSG_FATAL, "IpfixSender: ipfix_add_collector of %s:%d failed", ip, port);
		return;
	}
}


/**
 * looks in cached templates if given template is already registered there
 * @returns true if it was found
 */
bool IpfixSender::isTemplateRegistered(IpfixRecord::TemplateInfo* ti)
{
	list<boost::shared_ptr<IpfixRecord::TemplateInfo> >::iterator iter = registeredTemplates.begin();
	while (iter != registeredTemplates.end()) {
		if (iter->get()->templateId == ti->templateId) return true;
		iter++;
	}
	return false;
}

/**
 * removes given template from cached templates
 */
void IpfixSender::removeRegisteredTemplate(IpfixRecord::TemplateInfo* ti)
{
	list<boost::shared_ptr<IpfixRecord::TemplateInfo> >::iterator iter = registeredTemplates.begin();
	while (iter != registeredTemplates.end()) {
		if (iter->get()->templateId == ti->templateId) {
			registeredTemplates.erase(iter);
			return;
		}
		iter++;
	}
	THROWEXCEPTION("template with ID %d not found", ti->templateId);
}

/**
 * adds given template to the list of cached templates
 */
void IpfixSender::addRegisteredTemplate(boost::shared_ptr<IpfixRecord::TemplateInfo> ti)
{
	registeredTemplates.push_back(ti);
}

/**
 * Announces a new Template
 * @param sourceID ignored
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
void IpfixSender::onDataTemplate(IpfixDataTemplateRecord* record)
{
	boost::shared_ptr<IpfixRecord::DataTemplateInfo> dataTemplateInfo = record->dataTemplateInfo;
	uint16_t my_template_id;
	uint16_t my_preceding;
	ipfix_exporter* exporter = (ipfix_exporter*)ipfixExporter;
	
	if (!exporter) {
		THROWEXCEPTION("sndIpfix: Exporter not set");
	}
	
 	if (isTemplateRegistered(dataTemplateInfo.get())) {
 		// TODO: here we should check if both templates are the same, if they are, we do not need
 		// to inform ipfixlolib about it	
 		// we just assume that templates using the same id are identical		
 	} else {
 		addRegisteredTemplate(dataTemplateInfo);
 	}

	/* get or assign template ID */
	if(dataTemplateInfo->templateId)
	    my_template_id = dataTemplateInfo->templateId;
	else
	    my_template_id = dataTemplateInfo->templateId = ++lastTemplateId;

	my_preceding = dataTemplateInfo->preceding;
	if (lastTemplateId >= SENDER_TEMPLATE_ID_HI) {
		/* FIXME: Does not always work, e.g. if more than 50000 new Templates per minute are created */
		lastTemplateId = SENDER_TEMPLATE_ID_LOW;
	}
	
	int i;

	/* Count number of IPv4 fields with length 5 */
	int splitFields = 0;
	for (i = 0; i < dataTemplateInfo->fieldCount; i++) {
		IpfixRecord::FieldInfo* fi = &dataTemplateInfo->fieldInfo[i];
		if ((fi->type.id == IPFIX_TYPEID_sourceIPv4Address) && (fi->type.length == 5)) {
			splitFields++;
		}
		else if ((fi->type.id == IPFIX_TYPEID_destinationIPv4Address) && (fi->type.length == 5)) {
			splitFields++;
		}
	}

	/* Count number of IPv4 fields with length 5 */
	int splitFixedfields = 0;
	for (i = 0; i < dataTemplateInfo->dataCount; i++) {
		IpfixRecord::FieldInfo* fi = &dataTemplateInfo->dataInfo[i];
		if ((fi->type.id == IPFIX_TYPEID_sourceIPv4Address) && (fi->type.length == 5)) {
			splitFixedfields++;
		}
		else if ((fi->type.id == IPFIX_TYPEID_destinationIPv4Address) && (fi->type.length == 5)) {
			splitFixedfields++;
		}
	}

	if (0 != ipfix_start_datatemplate_set(exporter, my_template_id, my_preceding, dataTemplateInfo->fieldCount + splitFields, dataTemplateInfo->dataCount + splitFixedfields)) {
		THROWEXCEPTION("sndIpfix: ipfix_start_datatemplate_set failed");
	}

	for (i = 0; i < dataTemplateInfo->fieldCount; i++) {
		IpfixRecord::FieldInfo* fi = &dataTemplateInfo->fieldInfo[i];

		/* Split IPv4 fields with length 5, i.e. fields with network mask attached */
		if ((fi->type.id == IPFIX_TYPEID_sourceIPv4Address) && (fi->type.length == 5)) {
			ipfix_put_template_field(exporter, my_template_id, IPFIX_TYPEID_sourceIPv4Address, 4, 0);
			ipfix_put_template_field(exporter, my_template_id, IPFIX_TYPEID_sourceIPv4Mask, 1, 0);
		}
		else if ((fi->type.id == IPFIX_TYPEID_destinationIPv4Address) && (fi->type.length == 5)) {
			ipfix_put_template_field(exporter, my_template_id, IPFIX_TYPEID_destinationIPv4Address, 4, 0);
			ipfix_put_template_field(exporter, my_template_id, IPFIX_TYPEID_destinationIPv4Mask, 1, 0);
		}
		else {
			ipfix_put_template_field(exporter, my_template_id, fi->type.id, fi->type.length, fi->type.eid);
		}
	}

	DPRINTF("%d data fields", dataTemplateInfo->dataCount);

	int dataLength = 0;
	for (i = 0; i < dataTemplateInfo->dataCount; i++) {
		IpfixRecord::FieldInfo* fi = &dataTemplateInfo->dataInfo[i];

		dataLength += fi->type.length;

		/* Split IPv4 fields with length 5, i.e. fields with network mask attached */
		if ((fi->type.id == IPFIX_TYPEID_sourceIPv4Address) && (fi->type.length == 5)) {
			ipfix_put_template_fixedfield(exporter, my_template_id, IPFIX_TYPEID_sourceIPv4Address, 4, 0);
			ipfix_put_template_fixedfield(exporter, my_template_id, IPFIX_TYPEID_sourceIPv4Mask, 1, 0);
		}
		else if ((fi->type.id == IPFIX_TYPEID_destinationIPv4Address) && (fi->type.length == 5)) {
			ipfix_put_template_fixedfield(exporter, my_template_id, IPFIX_TYPEID_destinationIPv4Address, 4, 0);
			ipfix_put_template_fixedfield(exporter, my_template_id, IPFIX_TYPEID_destinationIPv4Mask, 1, 0);
		}
		else {
			ipfix_put_template_fixedfield(exporter, my_template_id, fi->type.id, fi->type.length, fi->type.eid);
		}
	}

	DPRINTF("%d data length", dataLength);

	char* data = (char*)dataLength?(char*)malloc(dataLength):0; // electric fence does not like 0-byte mallocs
	memcpy(data, dataTemplateInfo->data, dataLength);

	for (i = 0; i < dataTemplateInfo->dataCount; i++) {
		IpfixRecord::FieldInfo* fi = &dataTemplateInfo->dataInfo[i];

		/* Invert imask of IPv4 fields with length 5, i.e. fields with network mask attached */
		if ((fi->type.id == IPFIX_TYPEID_sourceIPv4Address) && (fi->type.length == 5)) {
			uint8_t* mask = (uint8_t*)(data + fi->offset + 4);
			*mask = 32 - *mask;
		}
		else if ((fi->type.id == IPFIX_TYPEID_destinationIPv4Address) && (fi->type.length == 5)) {
			uint8_t* mask = (uint8_t*)(data + fi->offset + 4);
			*mask = 32 - *mask;
		}
		else {
		}

	}

	if (0 != ipfix_put_template_data(exporter, my_template_id, data, dataLength)) {
		free(data);
		THROWEXCEPTION("sndIpfix: ipfix_put_template_data failed");
	}
	free(data);

	if (0 != ipfix_end_template_set(exporter, my_template_id)) {
		THROWEXCEPTION("sndIpfix: ipfix_end_template_set failed");
	}

	msg(MSG_INFO, "sndIpfix created template with ID %u", my_template_id);
	
	sendRecords();
}

/**
 * Invalidates a template; Does NOT free dataTemplateInfo
 * @param sourceID ignored
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
void IpfixSender::onDataTemplateDestruction(IpfixDataTemplateDestructionRecord* record)
{
	ipfix_exporter* exporter = (ipfix_exporter*)ipfixExporter;

	if (!exporter) {
		THROWEXCEPTION("exporter not set");
	}
	
	removeRegisteredTemplate(record->dataTemplateInfo.get());

	uint16_t my_template_id = record->dataTemplateInfo->templateId;


	/* Remove template from ipfixlolib */
	if (0 != ipfix_remove_template_set(exporter, my_template_id)) {
		msg(MSG_FATAL, "sndIpfix: ipfix_remove_template_set failed");
	}
	else
	{
		msg(MSG_INFO, "sndIpfix removed template with ID %u", my_template_id);
	}

	free(record->dataTemplateInfo->userData);
	
	sendRecords();
}


/**
 * Start generating a new Data Set unless the template ID is the same as the current Data Set.
 * Unfinished Data Set are terminated and sent if necessary.
 * @param templateId of the new Data Set
 * @return returns -1 on error, 0 otherwise
 */
void IpfixSender::startDataSet(uint16_t templateId) 
{
	ipfix_exporter* exporter = (ipfix_exporter*)ipfixExporter;
	uint16_t my_n_template_id = htons(templateId);
	
	/* check if we can use the current Data Set */
	//TODO: make maximum number of records per Data Set variable
	if((noCachedRecords < 10) && (templateId == currentTemplateId))
		return;

	if(noCachedRecords > 0)
		endAndSendDataSet();
	
	if (ipfix_start_data_set(exporter, my_n_template_id) != 0 ) {
		THROWEXCEPTION("sndIpfix: ipfix_start_data_set failed!");
	}

	currentTemplateId = templateId;
}
	

/**
 * Terminates and sends current Data Set if available.
 * @return returns -1 on error, 0 otherwise
 */
void IpfixSender::endAndSendDataSet() 
{
	if(noCachedRecords > 0) {
		ipfix_exporter* exporter = (ipfix_exporter*)ipfixExporter;
	
		if (ipfix_end_data_set(exporter, noCachedRecords) != 0) {
			THROWEXCEPTION("sndIpfix: ipfix_end_data_set failed");
		}
		
		// determine if we need to wait (we don't want to exceed the defined packet rate per second)
		// check in 100ms steps if maximum packet rate is reached - if yes, wait until the 100ms step
		// is over
		struct timeval tv;
		gettimeofday(&tv, 0);
		if ((tv.tv_sec==curTimeStep.tv_sec) && (tv.tv_usec/100000==curTimeStep.tv_usec/100000)) {			
			if (packetsSentStep>udpRateLimit/10) {
				// wait until current timestep is over
				usleep(100000-(tv.tv_usec%100000));				
			}
		} else {
			curTimeStep = tv;
			packetsSentStep = 0;
		}
		
		packetsSentStep++;

		if (ipfix_send(exporter) != 0) {
			THROWEXCEPTION("sndIpfix: ipfix_send failed");
		}

		removeRecordReferences();

		currentTemplateId = 0;
	}
}


/**
 * removes references to flows inside buffer recordsToRelease
 */
void IpfixSender::removeRecordReferences()
{
	while (!recordsToRelease.empty()) {
		recordsToRelease.front()->removeReference();
		recordsToRelease.pop();
	}
	noCachedRecords = 0;
}
	

/**
 * Put new Data Record in outbound exporter queue
 * @param rec Data Data Record
 */
void IpfixSender::onDataDataRecord(IpfixDataDataRecord* record)
{
	boost::shared_ptr<IpfixRecord::DataTemplateInfo> dataTemplateInfo = record->dataTemplateInfo;
	IpfixRecord::Data* data = record->data;
	ipfix_exporter* exporter = (ipfix_exporter*)ipfixExporter;

	if (!exporter) {
		THROWEXCEPTION("exporter not set");
	}

	startDataSet(dataTemplateInfo->templateId);

	int i;
	for (i = 0; i < dataTemplateInfo->fieldCount; i++) {
		IpfixRecord::FieldInfo* fi = &dataTemplateInfo->fieldInfo[i];

		/* Split IPv4 fields with length 5, i.e. fields with network mask attached */
		if ((fi->type.id == IPFIX_TYPEID_sourceIPv4Address) && (fi->type.length == 5)) {
			uint8_t* mask = &conversionRingbuffer[ringbufferPos++];
			*mask = 32 - *(uint8_t*)(data + fi->offset + 4);
			ipfix_put_data_field(exporter, data + fi->offset, 4);
			ipfix_put_data_field(exporter, mask, 1);
		}
		else if ((fi->type.id == IPFIX_TYPEID_destinationIPv4Address) && (fi->type.length == 5)) {
			uint8_t* mask = &conversionRingbuffer[ringbufferPos++];
			*mask = 32 - *(uint8_t*)(data + fi->offset + 4);
			ipfix_put_data_field(exporter, data + fi->offset, 4);
			ipfix_put_data_field(exporter, mask, 1);
		}
		else {
			if (fi->type.id == IPFIX_TYPEID_packetDeltaCount && fi->type.length<=8) {
				uint64_t p = 0;
				memcpy(&p, data+fi->offset, fi->type.length);
				statPacketsInFlows += ntohll(p);
			}
			ipfix_put_data_field(exporter, data + fi->offset, fi->type.length);
		}
	}
	 
	registerTimeout();

	statSentDataRecords++;
	
	recordsToRelease.push(record);
	
	noCachedRecords++;
	
	sendRecords();
}

/**
 * checks registered Templates if those are to be destroyed and destroys them if needed
 */
void IpfixSender::onReconfiguration2()
{
	ipfix_exporter* exporter = (ipfix_exporter*)ipfixExporter;
	
	if (!exporter) {
		THROWEXCEPTION("exporter not set");
	}
	
	list<boost::shared_ptr<IpfixRecord::TemplateInfo> >::iterator iter = registeredTemplates.begin();
	while (iter != registeredTemplates.end()) {
		if (iter->get()->destroyed) {
			uint16_t id = iter->get()->templateId;

			// Remove template from ipfixlolib 
			if (0 != ipfix_remove_template_set(exporter, id)) {
				msg(MSG_FATAL, "sndIpfix: ipfix_remove_template_set failed");
			} else {
				msg(MSG_INFO, "sndIpfix removed template with ID %u", id);
			}							
			
			iter = registeredTemplates.erase(iter);
		} else {
			iter++;
		}
	}
}


/**
 * sends records to the network
 * @param forcesend to send all records regardless how many were cached
 */
void IpfixSender::sendRecords(bool forcesend)
{
	if (noCachedRecords == 0) return;
	
	// TODO: extend ipfixlolib so that as many records as possible may be stored
	// in one network packet
	if ((noCachedRecords >= 10) || forcesend) {
		// send packet
		endAndSendDataSet();
		statSentPackets++;
		// set next timeout
		addToCurTime(&nextTimeout, recordCacheTimeout);
	}
}


/**
 * if flows are cached at the moment, this function sends them to the network
 * immediately
 */
void IpfixSender::flushPacket()
{
	sendRecords(true);
}


/**
 * gets called regularly to send data over the network
 */
void IpfixSender::onTimeout(void* dataPtr)
{
	timeoutRegistered = false;
	
	if (recordsAlreadySent) {
		timeval tv;
		gettimeofday(&tv, 0);
		if (nextTimeout.tv_sec>tv.tv_sec || (nextTimeout.tv_sec==tv.tv_sec && nextTimeout.tv_nsec>tv.tv_usec*1000)) {
			// next timeout is in the future, reregister it
			timer->addTimeout(this, nextTimeout, NULL);
			// as the next timeout is not over yet, we don't need to send the records
			return;
		}
	}
	sendRecords(true);
}

/**
 * registers timeout for function onTimeout in Timer
 * (used to send records which are cached)
 */
void IpfixSender::registerTimeout()
{
	if (timeoutRegistered) return;
	
	addToCurTime(&nextTimeout, recordCacheTimeout);
	timer->addTimeout(this, nextTimeout, NULL);
	timeoutRegistered = true;
}

/**
 * sends all cached records
 */
void IpfixSender::performShutdown()
{
	sendRecords(true);
}

/**
 * during reconfiguration ensure, that all cached flows are exported
 */
void IpfixSender::onReconfiguration1()
{
	sendRecords(true);
}

string IpfixSender::getStatisticsXML(double interval)
{
	char buf[200];
	snprintf(buf, ARRAY_SIZE(buf), "<totalSentDataRecords>%u</totalSentDataRecords><totalSentUDPDataRecordPackets>%u</totalSentUDPDataRecordPackets><totalPacketsInFlows>%u</totalPacketsInFlows>", 
			statSentDataRecords, statSentPackets, statPacketsInFlows);
	return buf;
}
