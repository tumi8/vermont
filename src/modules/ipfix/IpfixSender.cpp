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
#include "common/ipfixlolib/ipfix.h"

#include "common/msg.h"
#include "common/Time.h"
#include "core/Timer.h"

#include <sstream>
#include <stdexcept>
#include <string.h>

using namespace std;

/*
 * Valid Template Id range
 */
#define SENDER_TEMPLATE_ID_LOW 256
#define SENDER_TEMPLATE_ID_HI 65535

/**
 * Creates a new IPFIX Exporter. Do not forget to call @c startIpfixSender() to begin sending
 * @param sourceID Source ID this exporter will report
 * @param ip destination collector's address
 * @param port destination collector's port
 * @return handle to use when calling @c destroyIpfixSender()
 */
IpfixSender::IpfixSender(uint32_t observationDomainId, uint32_t maxRecordRate, uint32_t sctpDataLifetime, uint32_t sctpReconnectInterval,
		uint32_t templateRefreshInterval, uint32_t templateRefreshRate)
	: statSentPackets(0),
	  noCachedRecords(0),
	  recordCacheTimeout(IS_DEFAULT_RECORDCACHETIMEOUT),
	  timeoutRegistered(false),
	  recordsAlreadySent(false),
	  maxRecordRate(maxRecordRate)
{
	ipfix_exporter** exporterP = &this->ipfixExporter;
	statSentDataRecords = 0;
	statPacketsInFlows = 0;
	currentTemplateId = 0;

	nextTimeout.tv_sec = 0;
	nextTimeout.tv_nsec = 0;
	curTimeStep.tv_sec = 0;
	curTimeStep.tv_usec = 0;

	if(ipfix_init_exporter(observationDomainId, exporterP) != 0) {
		msg(MSG_FATAL, "IpfixSender: ipfix_init_exporter failed");
		goto out;
	}

	ipfix_set_sctp_lifetime(ipfixExporter, sctpDataLifetime);
	ipfix_set_sctp_reconnect_timer(ipfixExporter, sctpReconnectInterval);
	ipfix_set_template_transmission_timer(ipfixExporter, templateRefreshInterval);


	msg(MSG_DEBUG, "IpfixSender: running");
	return;

out:
	THROWEXCEPTION("IpfixSender creation failed");
	return;
}

IpfixSender::IpfixSender(uint32_t observationDomainId)
	: statSentPackets(0),
	  noCachedRecords(0),
	  recordCacheTimeout(IS_DEFAULT_RECORDCACHETIMEOUT),
	  timeoutRegistered(false),
	  recordsAlreadySent(false),
	  maxRecordRate(IS_DEFAULT_MAXRECORDRATE)
{
	ipfix_exporter** exporterP = &this->ipfixExporter;
	statSentDataRecords = 0;
	statPacketsInFlows = 0;
	currentTemplateId = 0;

	nextTimeout.tv_sec = 0;
	nextTimeout.tv_nsec = 0;
	curTimeStep.tv_sec = 0;
	curTimeStep.tv_usec = 0;

	if(ipfix_init_exporter(observationDomainId, exporterP) != 0) {
		msg(MSG_FATAL, "IpfixSender: ipfix_init_exporter failed");
		goto out;
	}

	ipfix_set_template_transmission_timer(ipfixExporter, IS_DEFAULT_TEMPLATE_TIMEINTERVAL);


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

	ipfix_deinit_exporter(ipfixExporter);
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
	switch(proto) {
	    case UDP:
	    	msg(MSG_INFO, "IpfixSender: adding UDP://%s:%d to exporter", ip, port);
	    	break;
	    case SCTP:
	    	msg(MSG_INFO, "IpfixSender: adding SCTP://%s:%d to exporter", ip, port);
	    	break;
#ifdef IPFIXLOLIB_RAWDIR_SUPPORT
	    case RAWDIR:
	    	msg(MSG_INFO, "IpfixSender: adding RAWDIR://%s to exporter", ip);
	    	break;
#endif
	    case TCP:
	        msg(MSG_INFO, "IpfixSender: adding TCP://%s:%d to exporter", ip, port);
	    default:
	    	THROWEXCEPTION("invalid protocol (%d) given!", proto);
	    	break;
	}

	if(ipfix_add_collector(ipfixExporter, ip, port, proto) != 0) {
		msg(MSG_FATAL, "IpfixSender: ipfix_add_collector of %s:%d failed", ip, port);
		return;
	}
}


/**
 * removes given template from cached templates
  */
void IpfixSender::removeRegisteredTemplate(boost::shared_ptr<TemplateInfo> ti)
{
       list<boost::shared_ptr<TemplateInfo> >::iterator iter = registeredTemplates.begin();
       while (iter != registeredTemplates.end()) {
               if (iter->get()->uniqueId == ti->uniqueId) {
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
void IpfixSender::addRegisteredTemplate(boost::shared_ptr<TemplateInfo> ti)
{
       registeredTemplates.push_back(ti);
}


/**
 * Get a small, unused Template Id
 * @returns unused Template Id or 0 if not available
 */
TemplateInfo::TemplateId IpfixSender::getUnusedTemplateId()
{
	TemplateInfo::TemplateId templateId = SENDER_TEMPLATE_ID_LOW;
	while(templateIdToUniqueId.find(templateId) != templateIdToUniqueId.end()) {
	    	templateId ++;
		if (templateId > SENDER_TEMPLATE_ID_HI)
			THROWEXCEPTION("IpfixSender: No unused Template Id available.");
	}
	return templateId;
}

/**
 * Announces a new Template
 * @param record Pointer to a structure defining the Template used
 */
void IpfixSender::onTemplate(IpfixTemplateRecord* record)
{
	boost::shared_ptr<TemplateInfo> dataTemplateInfo = record->templateInfo;
	// TODO: Implement Options Template handling
	if ((dataTemplateInfo->setId != TemplateInfo::IpfixTemplate) && (dataTemplateInfo->setId != TemplateInfo::IpfixDataTemplate))
	{
	    	msg(MSG_ERROR, "IpfixSender: Don't know how to handle Template (setId=%d)", dataTemplateInfo->setId);
		record->removeReference();
		return;
	}

	if (!ipfixExporter) {
		THROWEXCEPTION("IpfixSender: Exporter not set");
	}

	// check if this is a known template
	if(uniqueIdToTemplateId.find(dataTemplateInfo->uniqueId) != uniqueIdToTemplateId.end()) {
		msg(MSG_ERROR, "IpfixSender: Received known Template (id=%d) again, which should not happen.", dataTemplateInfo->templateId);
		record->removeReference();
		return;
	}

	/* get or assign Template ID */
	TemplateInfo::TemplateId my_template_id = 0;

	// if the Template ID included in the Template is available, we adopt it
	if(dataTemplateInfo->templateId) {
		if(templateIdToUniqueId.find(dataTemplateInfo->templateId) == templateIdToUniqueId.end()) {
			my_template_id = dataTemplateInfo->templateId;
		} else {
			msg(MSG_INFO, "IpfixSender: Template ID conflict, %d is already in use.", dataTemplateInfo->templateId);
		}
	}

	// generate new Template ID if necessary
	if(my_template_id == 0) {
		my_template_id = getUnusedTemplateId();
		msg(MSG_INFO, "IpfixSender: Use Template ID %d instead of %d.", my_template_id, dataTemplateInfo->templateId);
	}

	// Update maps
	templateIdToUniqueId[my_template_id] = dataTemplateInfo->uniqueId; 
	uniqueIdToTemplateId[dataTemplateInfo->uniqueId] = my_template_id;

	//for(map<TemplateInfo::TemplateId, uint16_t>::iterator iter = templateIdToUniqueId.begin(); iter != templateIdToUniqueId.end(); iter++) msg(MSG_FATAL, "template id %d -> unique id %d", iter->first, iter->second);
	//for(map<uint16_t, TemplateInfo::TemplateId>::iterator iter = uniqueIdToTemplateId.begin(); iter != uniqueIdToTemplateId.end(); iter++) msg(MSG_FATAL, "unique id %d -> template id %d", iter->first, iter->second);

	uint16_t my_preceding = 0;
	// Translate preceding template id if possible
	if(dataTemplateInfo->preceding) {
		map<TemplateInfo::TemplateId, uint16_t>::iterator iter = templateIdToUniqueId.find(dataTemplateInfo->preceding);
		if(iter == templateIdToUniqueId.end())
			msg(MSG_ERROR, "IpfixSender: Preceding Template (id=%d) not available, use zero instead", dataTemplateInfo->preceding);
		else
			my_preceding = uniqueIdToTemplateId[iter->second];
	}

	int i;

	/* Count number of IPv4 fields with length 5 */
	int splitFields = 0;
	for (i = 0; i < dataTemplateInfo->fieldCount; i++) {
		TemplateInfo::FieldInfo* fi = &dataTemplateInfo->fieldInfo[i];
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
		TemplateInfo::FieldInfo* fi = &dataTemplateInfo->dataInfo[i];
		if ((fi->type.id == IPFIX_TYPEID_sourceIPv4Address) && (fi->type.length == 5)) {
			splitFixedfields++;
		}
		else if ((fi->type.id == IPFIX_TYPEID_destinationIPv4Address) && (fi->type.length == 5)) {
			splitFixedfields++;
		}
	}

	if (0 != ipfix_start_datatemplate_set(ipfixExporter, my_template_id, my_preceding, dataTemplateInfo->fieldCount + splitFields, dataTemplateInfo->dataCount + splitFixedfields)) {
		THROWEXCEPTION("IpfixSender: ipfix_start_datatemplate_set failed");
	}

	for (i = 0; i < dataTemplateInfo->fieldCount; i++) {
		TemplateInfo::FieldInfo* fi = &dataTemplateInfo->fieldInfo[i];

		/* Split IPv4 fields with length 5, i.e. fields with network mask attached */
		if ((fi->type.id == IPFIX_TYPEID_sourceIPv4Address) && (fi->type.length == 5)) {
			ipfix_put_template_field(ipfixExporter, my_template_id, IPFIX_TYPEID_sourceIPv4Address, 4, 0);
			ipfix_put_template_field(ipfixExporter, my_template_id, IPFIX_TYPEID_sourceIPv4Mask, 1, 0);
		}
		else if ((fi->type.id == IPFIX_TYPEID_destinationIPv4Address) && (fi->type.length == 5)) {
			ipfix_put_template_field(ipfixExporter, my_template_id, IPFIX_TYPEID_destinationIPv4Address, 4, 0);
			ipfix_put_template_field(ipfixExporter, my_template_id, IPFIX_TYPEID_destinationIPv4Mask, 1, 0);
		}
		else {
			ipfix_put_template_field(ipfixExporter, my_template_id, fi->type.id, fi->type.length, fi->type.enterprise);
		}
	}

	DPRINTF("%d data fields", dataTemplateInfo->dataCount);

	int dataLength = 0;
	for (i = 0; i < dataTemplateInfo->dataCount; i++) {
		TemplateInfo::FieldInfo* fi = &dataTemplateInfo->dataInfo[i];

		dataLength += fi->type.length;

		/* Split IPv4 fields with length 5, i.e. fields with network mask attached */
		if ((fi->type.id == IPFIX_TYPEID_sourceIPv4Address) && (fi->type.length == 5)) {
			ipfix_put_template_fixedfield(ipfixExporter, my_template_id, IPFIX_TYPEID_sourceIPv4Address, 4, 0);
			ipfix_put_template_fixedfield(ipfixExporter, my_template_id, IPFIX_TYPEID_sourceIPv4Mask, 1, 0);
		}
		else if ((fi->type.id == IPFIX_TYPEID_destinationIPv4Address) && (fi->type.length == 5)) {
			ipfix_put_template_fixedfield(ipfixExporter, my_template_id, IPFIX_TYPEID_destinationIPv4Address, 4, 0);
			ipfix_put_template_fixedfield(ipfixExporter, my_template_id, IPFIX_TYPEID_destinationIPv4Mask, 1, 0);
		}
		else {
			ipfix_put_template_fixedfield(ipfixExporter, my_template_id, fi->type.id, fi->type.length, fi->type.enterprise);
		}
	}

	DPRINTF("%d data length", dataLength);

	char* data = (char*)dataLength?(char*)malloc(dataLength):0; // electric fence does not like 0-byte mallocs
	memcpy(data, dataTemplateInfo->data, dataLength);

	for (i = 0; i < dataTemplateInfo->dataCount; i++) {
		TemplateInfo::FieldInfo* fi = &dataTemplateInfo->dataInfo[i];

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

	if (0 != ipfix_put_template_data(ipfixExporter, my_template_id, data, dataLength)) {
		free(data);
		THROWEXCEPTION("IpfixSender: ipfix_put_template_data failed");
	}
	free(data);

	if (0 != ipfix_end_template_set(ipfixExporter, my_template_id)) {
		THROWEXCEPTION("IpfixSender: ipfix_end_template_set failed");
	}

	msg(MSG_INFO, "IpfixSender: created template with ID %u", my_template_id);

	sendRecords();

	// register Template 
	addRegisteredTemplate(dataTemplateInfo);

	record->removeReference();
}

/**
 * Invalidates a template; Does NOT free dataTemplateInfo
 * @param record Pointer to a structure defining the Template used
 */
void IpfixSender::onTemplateDestruction(IpfixTemplateDestructionRecord* record)
{
	boost::shared_ptr<TemplateInfo> dataTemplateInfo = record->templateInfo;
	// TODO: Implement Options Template handling
	if ((dataTemplateInfo->setId != TemplateInfo::IpfixTemplate) && (dataTemplateInfo->setId != TemplateInfo::IpfixDataTemplate))
	{
		msg(MSG_ERROR, "IpfixSender: Don't know how to handle Template (setId=%d)", dataTemplateInfo->setId);
		record->removeReference();
		return;
	}

	if (!ipfixExporter) {
		THROWEXCEPTION("ipfixExporter not set");
	}

	map<uint16_t, TemplateInfo::TemplateId>::iterator iter = uniqueIdToTemplateId.find(dataTemplateInfo->uniqueId);
	if(iter == templateIdToUniqueId.end()) {
		msg(MSG_ERROR, "IpfixSender: Template (id=%d) to be destroyed does not exist.", dataTemplateInfo->templateId);
		record->removeReference();
		return;
	}

	TemplateInfo::TemplateId my_template_id = iter->second;

	// remove from maps
	uniqueIdToTemplateId.erase(iter);
	templateIdToUniqueId.erase(my_template_id);

	// send remaining records first
	sendRecords();

	/* Remove template from ipfixlolib */
	if (0 != ipfix_remove_template_set(ipfixExporter, my_template_id)) {
		msg(MSG_FATAL, "IpfixSender: ipfix_remove_template_set failed");
	}
	else
	{
		msg(MSG_INFO, "IpfixSender: removed template with ID %u", my_template_id);
	}

	// enforce sending the withdrawal message
	if (ipfix_send(ipfixExporter) != 0) {
		THROWEXCEPTION("IpfixSender: ipfix_send failed");
	}

	// unregister Template 
	removeRegisteredTemplate(dataTemplateInfo);

	record->removeReference();
}


/**
 * Start generating a new Data Set unless the template ID is the same as the current Data Set.
 * Unfinished Data Set are terminated and sent if necessary.
 * @param templateId of the new Data Set
 * @return returns -1 on error, 0 otherwise
 */
void IpfixSender::startDataSet(TemplateInfo::TemplateId templateId)
{
	uint16_t my_n_template_id = htons(templateId);

	/* check if we can use the current Data Set */
	//TODO: make maximum number of records per Data Set variable
	if((noCachedRecords < 10) && (templateId == currentTemplateId))
		return;

	if(noCachedRecords > 0)
		endAndSendDataSet();

	if (ipfix_start_data_set(ipfixExporter, my_n_template_id) != 0 ) {
		THROWEXCEPTION("IpfixSender: ipfix_start_data_set failed!");
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
		if (ipfix_end_data_set(ipfixExporter, noCachedRecords) != 0) {
			THROWEXCEPTION("IpfixSender: ipfix_end_data_set failed");
		}

		// determine if we need to wait (we don't want to exceed the defined packet rate per second)
		// check in 100ms steps if maximum packet rate is reached - if yes, wait until the 100ms step
		// is over
		struct timeval tv;
		gettimeofday(&tv, 0);
		if ((tv.tv_sec==curTimeStep.tv_sec) && (tv.tv_usec/100000==curTimeStep.tv_usec/100000)) {
			if (recordsSentStep>maxRecordRate/10) {
				// wait until current timestep is over
				usleep(100000-(tv.tv_usec%100000));
			}
		} else {
			curTimeStep = tv;
			recordsSentStep = 0;
		}

		if (ipfix_send(ipfixExporter) != 0) {
			THROWEXCEPTION("IpfixSender: ipfix_send failed");
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
 * @param rec Data Record
 */
void IpfixSender::onDataRecord(IpfixDataRecord* record)
{
	boost::shared_ptr<TemplateInfo> dataTemplateInfo = record->templateInfo;
	// TODO: Implement Options Data Record handling
	if ((dataTemplateInfo->setId != TemplateInfo::IpfixTemplate) && (dataTemplateInfo->setId != TemplateInfo::IpfixDataTemplate))
	{
	    	msg(MSG_ERROR, "IpfixSender: Don't know how to handle Template (setId=%d)", dataTemplateInfo->setId);
		record->removeReference();
		return;
	}

	IpfixRecord::Data* data = record->data;

	if (!ipfixExporter) {
		THROWEXCEPTION("ipfixExporter not set");
	}

	startDataSet(dataTemplateInfo->templateId);

	int i;
	for (i = 0; i < dataTemplateInfo->fieldCount; i++) {
		TemplateInfo::FieldInfo* fi = &dataTemplateInfo->fieldInfo[i];

		/* Split IPv4 fields with length 5, i.e. fields with network mask attached */
		if ((fi->type.id == IPFIX_TYPEID_sourceIPv4Address) && (fi->type.length == 5)) {
			uint8_t* mask = &conversionRingbuffer[ringbufferPos++];
			*mask = 32 - *(uint8_t*)(data + fi->offset + 4);
			ipfix_put_data_field(ipfixExporter, data + fi->offset, 4);
			ipfix_put_data_field(ipfixExporter, mask, 1);
		}
		else if ((fi->type.id == IPFIX_TYPEID_destinationIPv4Address) && (fi->type.length == 5)) {
			uint8_t* mask = &conversionRingbuffer[ringbufferPos++];
			*mask = 32 - *(uint8_t*)(data + fi->offset + 4);
			ipfix_put_data_field(ipfixExporter, data + fi->offset, 4);
			ipfix_put_data_field(ipfixExporter, mask, 1);
		}
		else {
			if (fi->type.id == IPFIX_TYPEID_packetDeltaCount && fi->type.length<=8) {
				uint64_t p = 0;
				memcpy(&p, data+fi->offset, fi->type.length);
				statPacketsInFlows += ntohll(p);
			}
			ipfix_put_data_field(ipfixExporter, data + fi->offset, fi->type.length);
		}
	}

	registerTimeout();

	statSentDataRecords++;
	recordsSentStep++;

	recordsToRelease.push(record);

	noCachedRecords++;

	sendRecords();
}

/**
 * checks registered Templates if those are to be destroyed and destroys them if needed
 */
void IpfixSender::onReconfiguration2()
{
       if (!ipfixExporter) {
               THROWEXCEPTION("ipfixExporter not set");
       }

       list<boost::shared_ptr<TemplateInfo> >::iterator iter = registeredTemplates.begin();
       while (iter != registeredTemplates.end()) {
		if (iter->get()->destroyed) {
			
			map<uint16_t, TemplateInfo::TemplateId>::iterator iter2 = uniqueIdToTemplateId.find(iter->get()->uniqueId);
			if(iter2 == uniqueIdToTemplateId.end()) {
				THROWEXCEPTION("IpfixSender: corrupt uniqueIdToTemplate");
			}

			TemplateInfo::TemplateId id = iter2->second;

			// Remove template from ipfixlolib
			if (0 != ipfix_remove_template_set(ipfixExporter, id)) {
				msg(MSG_FATAL, "IpfixSender: ipfix_remove_template_set failed");
			} else {
				msg(MSG_INFO, "IpfixSender: removed template with ID %u", id);
			}

			// remove from maps
			uniqueIdToTemplateId.erase(iter2);
			templateIdToUniqueId.erase(id);

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
	}
	// set next timeout
	addToCurTime(&nextTimeout, recordCacheTimeout);
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
	// send remaining records first
	sendRecords(true);

	// Destroy Templates
	for(map<TemplateInfo::TemplateId, uint16_t>::iterator iter = templateIdToUniqueId.begin(); iter != templateIdToUniqueId.end(); iter++) {
		/* Remove template from ipfixlolib */
		if (0 != ipfix_remove_template_set(ipfixExporter, iter->first)) {
			msg(MSG_FATAL, "IpfixSender: ipfix_remove_template_set failed");
		}
		else
		{
			msg(MSG_INFO, "IpfixSender: removed template with ID %u", iter->first);
		}
	}
	
	// enforce sending the withdrawal message
	if (ipfix_send(ipfixExporter) != 0) {
		THROWEXCEPTION("IpfixSender: ipfix_send failed");
	}
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

