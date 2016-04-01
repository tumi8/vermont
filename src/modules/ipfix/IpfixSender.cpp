/*
 * IPFIX Concentrator Module Library
 * Copyright (C) 2004 Christoph Sommer <http://www.deltadevelopment.de/users/christoph/ipfix/>
 * Copyright (C) 2014 Oliver Gasser
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
 * Creates a new IPFIX Exporter. Do not forget to call @c startIpfixSender() to
 * begin sending
 * @param sourceID Source ID this exporter will report
 * @param ip destination collector's address
 * @param port destination collector's port
 * @return handle to use when calling @c destroyIpfixSender()
 */
IpfixSender::IpfixSender(uint32_t observationDomainId, uint32_t maxRecordRate,
		uint32_t sctpDataLifetime, uint32_t sctpReconnectInterval,
		uint32_t templateRefreshInterval,
		const std::string &certificateChainFile,
		const std::string &privateKeyFile,
		const std::string &caFile,
		const std::string &caPath,
		export_protocol_version export_protocol)

	: statSentDataRecords(0),
	  statSentPackets(0),	  
	  statPacketsInFlows(0),
	  noCachedRecords(0),
	  noRecordsInCurrentSet(0),
	  recordCacheTimeout(IS_DEFAULT_RECORDCACHETIMEOUT),
	  timeoutRegistered(false),
	  currentTemplateId(0),
	  maxRecordRate(maxRecordRate),
	  export_protocol(export_protocol)
{
	const char *certificate_chain_file = NULL;
	const char *private_key_file = NULL;
	const char *ca_file = NULL;
	const char *ca_path = NULL;

	ipfix_exporter** exporterP = &this->ipfixExporter;

	nextTimeout.tv_sec = 0;
	nextTimeout.tv_nsec = 0;
	curTimeStep.tv_sec = 0;
	curTimeStep.tv_usec = 0;

	if(ipfix_init_exporter(export_protocol, observationDomainId, exporterP) != 0) {
		msg(MSG_FATAL, "IpfixSender: ipfix_init_exporter failed");
		goto out;
	}

	ipfix_set_sctp_lifetime(ipfixExporter, sctpDataLifetime);
	ipfix_set_sctp_reconnect_timer(ipfixExporter, sctpReconnectInterval);
	ipfix_set_template_transmission_timer(ipfixExporter, templateRefreshInterval);

 
	if ( ! certificateChainFile.empty())
		certificate_chain_file = certificateChainFile.c_str();
	if ( ! privateKeyFile.empty())
		private_key_file = privateKeyFile.c_str();
	/* Private key will be searched for in the certificate chain file if
	 * no private key file is set */
	if (certificate_chain_file || private_key_file)
		ipfix_set_dtls_certificate(ipfixExporter,
				certificate_chain_file, private_key_file);

	if ( ! caFile.empty() ) ca_file = caFile.c_str();
	if ( ! caPath.empty() ) ca_path = caPath.c_str();
	if (ca_file || ca_path)
		ipfix_set_ca_locations(ipfixExporter, ca_file, ca_path);

	msg(MSG_DEBUG, "IpfixSender: running");
	return;

out:
	THROWEXCEPTION("IpfixSender creation failed");
	return;
}

IpfixSender::IpfixSender(uint32_t observationDomainId, uint32_t maxRecordRate)
	: statSentDataRecords(0),
	  statSentPackets(0),	  
	  statPacketsInFlows(0),
	  noCachedRecords(0),
	  recordCacheTimeout(IS_DEFAULT_RECORDCACHETIMEOUT),
	  timeoutRegistered(false),
	  currentTemplateId(0),
	  maxRecordRate(maxRecordRate),
	  export_protocol(IPFIX_PROTOCOL)
{
	ipfix_exporter** exporterP = &this->ipfixExporter;

	nextTimeout.tv_sec = 0;
	nextTimeout.tv_nsec = 0;
	curTimeStep.tv_sec = 0;
	curTimeStep.tv_usec = 0;

	if(ipfix_init_exporter(export_protocol, observationDomainId, exporterP) != 0) {
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
	this->shutdown(false);
	ipfix_deinit_exporter(&ipfixExporter);
}

/**
 * Add another IPFIX collector to export the stream to
 * the lowlevel stuff in handled by underlying ipfixlolib
 * @param ips handle to the Exporter
 * @param ip string of the IP
 * @param port port number
 * @param aux_config additional configuration details required for UDP,
 * 	DTLS_OVER_UDP and DTLS_OVER_SCTP. See ipfixlolib documentation for more
 * 	information.
 * FIXME: support for other than UDP
 */
void IpfixSender::addCollector(const char *ip, uint16_t port,
		ipfix_transport_protocol proto, void *aux_config)
{
	ipfix_exporter *ex = (ipfix_exporter *)ipfixExporter;

	switch(proto) {
	    case UDP:
	    	msg(MSG_INFO, "IpfixSender: adding UDP://%s:%d to exporter",
				ip, port);
	    	break;
	    case SCTP:
	    	msg(MSG_INFO, "IpfixSender: adding SCTP://%s:%d to exporter",
				ip, port);
	    	break;
#ifdef IPFIXLOLIB_RAWDIR_SUPPORT
	    case RAWDIR:
	    	msg(MSG_INFO, "IpfixSender: adding RAWDIR://%s to exporter",
				ip);
	    	break;
#endif
#ifdef SUPPORT_DTLS
	    case DTLS_OVER_UDP:
	    	msg(MSG_INFO,
			"IpfixSender: adding DTLS over UDP://%s:%d to exporter",
			ip, port);
	    	break;
#endif
#ifdef SUPPORT_DTLS_OVER_SCTP
	    case DTLS_OVER_SCTP:
	    	msg(MSG_INFO,
		"IpfixSender: adding DTLS over SCTP://%s:%d to exporter",
			ip, port);
	    	break;
#endif
	    case TCP:
	        msg(MSG_INFO, "IpfixSender: adding TCP://%s:%d to exporter",
				ip, port);
	    default:
	    	THROWEXCEPTION("invalid protocol (%d) given!", proto);
	    	break;
	}

	if(ipfix_add_collector(ex, ip, port, proto, aux_config) != 0) {
		msg(MSG_FATAL,
			"IpfixSender: ipfix_add_collector of %s:%d failed",
			ip, port);
		return;
	}
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
		if (templateId == SENDER_TEMPLATE_ID_HI)
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
	    	msg(MSG_ERROR, "IpfixSender: Don't know how to handle Template (setId=%u)", dataTemplateInfo->setId);
		record->removeReference();
		return;
	}

	if (!ipfixExporter) {
		THROWEXCEPTION("IpfixSender: Exporter not set");
	}

	msg(MSG_DEBUG, "IpfixSender: Template received (setid=%u, id=%u)", (uint16_t)(dataTemplateInfo->setId), dataTemplateInfo->templateId);

	// get message lock
	ipfixMessageLock.lock();
	// return if exitFlag has ben set in the meanwhile
	if (exitFlag) {
		record->removeReference();
		ipfixMessageLock.unlock();
		return;
	}


	// check if this is a known template
	if(uniqueIdToTemplateId.find(dataTemplateInfo->getUniqueId()) != uniqueIdToTemplateId.end()) {
		msg(MSG_ERROR, "IpfixSender: Received known Template (id=%u) again, which should not happen.", dataTemplateInfo->templateId);
		record->removeReference();
		ipfixMessageLock.unlock();
		return;
	}

	/* get or assign Template ID */
	TemplateInfo::TemplateId my_template_id = 0;

	// if the Template ID included in the Template is available, we adopt it
	if(dataTemplateInfo->templateId) {
		if(templateIdToUniqueId.find(dataTemplateInfo->templateId) == templateIdToUniqueId.end()) {
			my_template_id = dataTemplateInfo->templateId;
		} else {
			msg(MSG_INFO, "IpfixSender: Template ID conflict, %u is already in use.", dataTemplateInfo->templateId);
		}
	}

	// generate new Template ID if necessary
	if(my_template_id == 0) {
		my_template_id = getUnusedTemplateId();
		msg(MSG_INFO, "IpfixSender: Use Template ID %u instead of %u.", my_template_id, dataTemplateInfo->templateId);
	}

	// Update maps
	templateIdToUniqueId[my_template_id] = dataTemplateInfo->getUniqueId(); 
	uniqueIdToTemplateId[dataTemplateInfo->getUniqueId()] = my_template_id;

	//for(map<TemplateInfo::TemplateId, uint16_t>::iterator iter = templateIdToUniqueId.begin(); iter != templateIdToUniqueId.end(); iter++) msg(MSG_FATAL, "template id %u -> unique id %u", iter->first, iter->second);
	//for(map<uint16_t, TemplateInfo::TemplateId>::iterator iter = uniqueIdToTemplateId.begin(); iter != uniqueIdToTemplateId.end(); iter++) msg(MSG_FATAL, "unique id %u -> template id %u", iter->first, iter->second);

	uint16_t my_preceding = 0;
	// Translate preceding template id if possible
	if(dataTemplateInfo->preceding) {
		map<TemplateInfo::TemplateId, uint16_t>::iterator iter = templateIdToUniqueId.find(dataTemplateInfo->preceding);
		if(iter == templateIdToUniqueId.end())
			msg(MSG_ERROR, "IpfixSender: Preceding Template (id=%u) not available, use zero instead", dataTemplateInfo->preceding);
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

	if (0 != ipfix_start_datatemplate(ipfixExporter, my_template_id, my_preceding, dataTemplateInfo->fieldCount + splitFields, dataTemplateInfo->dataCount + splitFixedfields)) {
		THROWEXCEPTION("IpfixSender: ipfix_start_datatemplate failed");
	}

	for (i = 0; i < dataTemplateInfo->fieldCount; i++) {
		TemplateInfo::FieldInfo* fi = &dataTemplateInfo->fieldInfo[i];

		/* Split IPv4 fields with length 5, i.e. fields with network mask attached */
		if ((fi->type.id == IPFIX_TYPEID_sourceIPv4Address) && (fi->type.length == 5)) {
			ipfix_put_template_field(ipfixExporter, my_template_id, IPFIX_TYPEID_sourceIPv4Address, 4, 0);
			ipfix_put_template_field(ipfixExporter, my_template_id, IPFIX_TYPEID_sourceIPv4PrefixLength, 1, 0);
		}
		else if ((fi->type.id == IPFIX_TYPEID_destinationIPv4Address) && (fi->type.length == 5)) {
			ipfix_put_template_field(ipfixExporter, my_template_id, IPFIX_TYPEID_destinationIPv4Address, 4, 0);
			ipfix_put_template_field(ipfixExporter, my_template_id, IPFIX_TYPEID_destinationIPv4PrefixLength, 1, 0);
		}
		else {
			ipfix_put_template_field(ipfixExporter, my_template_id, fi->type.id, fi->type.length, fi->type.enterprise);
		}
	}

	DPRINTF("%u data fields", dataTemplateInfo->dataCount);

	int dataLength = 0;
	for (i = 0; i < dataTemplateInfo->dataCount; i++) {
		TemplateInfo::FieldInfo* fi = &dataTemplateInfo->dataInfo[i];

		dataLength += fi->type.length;

		/* Split IPv4 fields with length 5, i.e. fields with network mask attached */
		if ((fi->type.id == IPFIX_TYPEID_sourceIPv4Address) && (fi->type.length == 5)) {
			ipfix_put_template_fixedfield(ipfixExporter, my_template_id, IPFIX_TYPEID_sourceIPv4Address, 4, 0);
			ipfix_put_template_fixedfield(ipfixExporter, my_template_id, IPFIX_TYPEID_sourceIPv4PrefixLength, 1, 0);
		}
		else if ((fi->type.id == IPFIX_TYPEID_destinationIPv4Address) && (fi->type.length == 5)) {
			ipfix_put_template_fixedfield(ipfixExporter, my_template_id, IPFIX_TYPEID_destinationIPv4Address, 4, 0);
			ipfix_put_template_fixedfield(ipfixExporter, my_template_id, IPFIX_TYPEID_destinationIPv4PrefixLength, 1, 0);
		}
		else {
			ipfix_put_template_fixedfield(ipfixExporter, my_template_id, fi->type.id, fi->type.length, fi->type.enterprise);
		}
	}

	DPRINTF("%u data length", dataLength);

	char* data = dataLength?(char*)malloc(dataLength):0; // electric fence does not like 0-byte mallocs
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

	// Only add Data Template data if there is any
	if (data && ipfix_put_template_data(ipfixExporter, my_template_id, data, dataLength)) {
		THROWEXCEPTION("IpfixSender: ipfix_put_template_data failed");
	}
	free(data);

	if (0 != ipfix_end_template(ipfixExporter, my_template_id)) {
		THROWEXCEPTION("IpfixSender: ipfix_end_template failed");
	}

	msg(MSG_INFO, "IpfixSender: created template with ID %u", my_template_id);

	// release message lock
	ipfixMessageLock.unlock();

	// we want the templates to be sent to the collector
	// Gerhard (09/2010): 
	// - commented out because this would cause multiple sendings of Templates at startup in the case of UDP
	// - anyway, Templates will allways be send in front of with Data Records if necessary
	// - an advantage of calling sendRecords() here would be that we can detect if the UDP collector is offline
	//sendRecords(Always);

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
		msg(MSG_ERROR, "IpfixSender: Don't know how to handle Template (setId=%u)", dataTemplateInfo->setId);
		record->removeReference();
		return;
	}

	if (!ipfixExporter) {
		THROWEXCEPTION("ipfixExporter not set");
	}

	// send remaining records first
	sendRecords(IfNotEmpty);

	msg(MSG_DEBUG, "IpfixSender: Template destruction received (setid=%u, id=%u)", (uint16_t)(dataTemplateInfo->setId), dataTemplateInfo->templateId);

	// get message lock
	ipfixMessageLock.lock();
	// return if exitFlag has ben set in the meanwhile
	if (exitFlag) {
		record->removeReference();
		ipfixMessageLock.unlock();
		return;
	}

	map<uint16_t, TemplateInfo::TemplateId>::iterator iter = uniqueIdToTemplateId.find(dataTemplateInfo->getUniqueId());
	if(iter == uniqueIdToTemplateId.end()) {
		msg(MSG_ERROR, "IpfixSender: Template (id=%u) to be destroyed does not exist.", dataTemplateInfo->templateId);
		record->removeReference();
		ipfixMessageLock.unlock();
		return;
	}

	TemplateInfo::TemplateId my_template_id = iter->second;

	// remove from maps
	uniqueIdToTemplateId.erase(iter);
	templateIdToUniqueId.erase(my_template_id);

	/* Remove template from ipfixlolib */
	if (0 != ipfix_remove_template(ipfixExporter, my_template_id)) {
		msg(MSG_FATAL, "IpfixSender: ipfix_remove_template failed");
	}
	else
	{
		msg(MSG_INFO, "IpfixSender: removed template with ID %u", my_template_id);
	}

	// enforce sending the withdrawal message
	if (ipfix_send(ipfixExporter) != 0) {
		THROWEXCEPTION("IpfixSender: ipfix_send failed");
	}
	msg(MSG_INFO, "IpfixSender: destroyed template with ID %u", my_template_id);

	// release message lock
	ipfixMessageLock.unlock();

	// we want the templates to be sent to the collector
	sendRecords(Always);

	record->removeReference();
}


/**
 * Start generating a new Data Set unless the template ID is the same as the current Data Set.
 * Unfinished Data Set are terminated and sent if necessary.
 * @param templateId of the new Data Set
 * @return returns -1 on error, 0 otherwise
 */
void IpfixSender::setTemplateId(TemplateInfo::TemplateId templateId, uint16_t dataLength)
{
	ipfix_exporter* exporter = (ipfix_exporter*)ipfixExporter;
	uint16_t my_n_template_id = htons(templateId);
	if (currentTemplateId == 0)
		; /* Do nothing */
	else if(templateId != currentTemplateId) {
		endDataSet();
		if (remainingSpace < dataLength + IPFIX_OVERHEAD_PER_SET) {
			send();
		}
	} else if (remainingSpace < dataLength) {
		endDataSet();
		send();
	} else {
		return;
	}
		
	if (ipfix_start_data_set(exporter, my_n_template_id) != 0 ) {
		THROWEXCEPTION("ipfix_start_data_set failed!");
	}
	remainingSpace = ipfix_get_remaining_space(exporter);
	currentTemplateId = templateId;
}

/**
 * Terminates current Data Set.
 * @return returns -1 on error, 0 otherwise
 */
void IpfixSender::endDataSet()
{
	if (ipfix_end_data_set(ipfixExporter, noRecordsInCurrentSet) != 0) {
		THROWEXCEPTION("ipfix_end_data_set failed");
	}
	noRecordsInCurrentSet = 0;
	currentTemplateId = 0;
}

void IpfixSender::send() {

	// determine if we need to wait (we don't want to exceed the defined packet rate per second)
	// check in 100ms steps if maximum packet rate is reached - if yes, wait until the 100ms step
	// is over
	// do only perform limiting of the maxRecordRate is > 0
	if (maxRecordRate > 0) {
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
	}

	if (ipfix_send(ipfixExporter) != 0) {
		THROWEXCEPTION("sndIpfix: ipfix_send failed");
	}

	removeRecordReferences();
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
	    	msg(MSG_ERROR, "IpfixSender: Don't know how to handle Template (setId=%u)", dataTemplateInfo->setId);
		record->removeReference();
		return;
	}

	if (!ipfixExporter) {
		THROWEXCEPTION("ipfixExporter not set");
	}

	// get the message lock
	ipfixMessageLock.lock();

	// check if we know the Template
	map<uint16_t, TemplateInfo::TemplateId>::iterator iter = uniqueIdToTemplateId.find(dataTemplateInfo->getUniqueId());
	if(iter == uniqueIdToTemplateId.end()) {
		msg(MSG_ERROR, "IpfixSender: Discard Data Record because Template (id=%u) does not exist (this may happen during reconfiguration).", dataTemplateInfo->templateId);
		record->removeReference();
		ipfixMessageLock.unlock();
		return;
	}

	IpfixRecord::Data* data = record->data;
	TemplateInfo::TemplateId my_template_id = iter->second;

	// return if exitFlag has ben set in the meanwhile
	if (exitFlag) {
		record->removeReference();
		ipfixMessageLock.unlock();
		return;
	}

	setTemplateId(my_template_id, record->dataLength);

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
	remainingSpace -= record->dataLength;
	statSentDataRecords++;
	recordsSentStep++;

	recordsToRelease.push(record);

	noCachedRecords++;
	noRecordsInCurrentSet++;
	registerTimeout();

	// release the message lock
	ipfixMessageLock.unlock();
}

/**
 * checks registered Templates if those are to be destroyed and destroys them if needed
 */
void IpfixSender::onReconfiguration2()
{
	if (!ipfixExporter) {
		THROWEXCEPTION("ipfixExporter not set");
	}

	// send cached records
	sendRecords(IfNotEmpty);

	// get message lock
	ipfixMessageLock.lock();

	// Destroy all templates (they will be resent after reconfiguration if necessary)
	for(map<TemplateInfo::TemplateId, uint16_t>::iterator iter = templateIdToUniqueId.begin(); iter != templateIdToUniqueId.end(); iter++) {
		/* Remove template from ipfixlolib */
		if (0 != ipfix_remove_template(ipfixExporter, iter->first)) {
			msg(MSG_FATAL, "IpfixSender: ipfix_remove_template failed");
		}
		else
		{
			msg(MSG_INFO, "IpfixSender: removed template with ID %u", iter->first);
		}
	}
	// clear maps
	uniqueIdToTemplateId.clear();
	templateIdToUniqueId.clear();

	// release message lock
	ipfixMessageLock.unlock();

	sendRecords(Always);

}


/**
 * sends records to the network
 * @param policy controls when to send (IfFull: only if Data Set is full, IfNotEmpty: only if Data Set is not empty, Always: send always (useful for sending Templates))
 */
void IpfixSender::sendRecords(SendPolicy policy)
{
	if ((noCachedRecords == 0) && (policy != Always)) return;

	// get the message lock
	ipfixMessageLock.lock();

	// check again
	if ((noCachedRecords == 0) && (policy != Always)) {
		ipfixMessageLock.unlock();
		return;
	}

	// We cancel the timeout because we're about to send
	// out all records.
	timeoutRegistered = false;
	// send packet
	if (currentTemplateId) endDataSet();
	send();
	statSentPackets++;

	// get the message lock
	ipfixMessageLock.unlock();

}

/**
 * gets called regularly to send data over the network
 */
void IpfixSender::onSendRecordsTimeout(void) {

	if (!timeoutRegistered) return;
	timeval tv;
	gettimeofday(&tv, 0);
	if (nextTimeout.tv_sec<tv.tv_sec || (nextTimeout.tv_sec==tv.tv_sec && nextTimeout.tv_nsec<tv.tv_usec*1000)) {
		DPRINTF("Sending Records due to timeout.");
		sendRecords(Always);
	}
}
void IpfixSender::onBeatTimeout(void) {
	ipfix_beat(ipfixExporter);
}
void IpfixSender::onTimeout(void* dataPtr)
{
	onSendRecordsTimeout();
	onBeatTimeout();
	registerBeatTimeout();
}

/**
 * registers timeout for function onTimeout in Timer
 * (used to send records which are cached)
 */
void IpfixSender::registerTimeout()
{
	if (timeoutRegistered) return;
	addToCurTime(&nextTimeout, recordCacheTimeout);
	timeoutRegistered = true;
}

void IpfixSender::registerBeatTimeout()
{
	timespec to;
	addToCurTime(&to, 100);
	timer->addTimeout(this, to, &timeoutIpfixlolibBeat);
}

/**
 * 
 */
void IpfixSender::notifyQueueRunning() {
	registerBeatTimeout();
}

/**
 * sends all cached records
 */
void IpfixSender::performShutdown()
{
	// send remaining records first
	sendRecords(IfNotEmpty);
}

/**
 * during reconfiguration
 */
void IpfixSender::onReconfiguration1()
{
}

string IpfixSender::getStatisticsXML(double interval)
{
	char buf[200];
	snprintf(buf, ARRAY_SIZE(buf), "<totalSentDataRecords>%u</totalSentDataRecords><totalSentUDPDataRecordPackets>%u</totalSentUDPDataRecordPackets><totalPacketsInFlows>%u</totalPacketsInFlows>",
			statSentDataRecords, statSentPackets, statPacketsInFlows);
	return buf;
}

