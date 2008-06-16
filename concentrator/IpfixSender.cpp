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

#include <stdexcept>
#include <string.h>
#include "IpfixSender.hpp"
#include "ipfix.hpp"

#include "common/msg.h"

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
IpfixSender::IpfixSender(uint16_t observationDomainId, const char* ip, uint16_t port, ipfix_transport_protocol proto) {
	setSinkOwner("IpfixSender");
	ipfix_exporter** exporterP = &this->ipfixExporter;
	sentRecords = 0;
	recordsInDataSet = 0;
	currentTemplateId = 0;
	lastTemplateId = SENDER_TEMPLATE_ID_LOW;

	if(ipfix_init_exporter(observationDomainId, exporterP) != 0) {
		msg(MSG_FATAL, "sndIpfix: ipfix_init_exporter failed");
		goto out;
	}

	if (ip && port) {
		if(addCollector(ip, port, proto) != 0) {
			goto out1;
		}
	}
	
        msg(MSG_DEBUG, "IpfixSender: running");

	return;
	
out1:
	ipfix_deinit_exporter(*exporterP);
out:
	THROWEXCEPTION("IpfixSender creation failed");
	return;	
}

/**
 * Removes a collector from the list of Collectors to send Records to
 */
IpfixSender::~IpfixSender() {
	ipfix_exporter* exporter = (ipfix_exporter*)ipfixExporter;

	ipfix_deinit_exporter(exporter);
}

/**
 * Starts or resumes sending messages
 */
void IpfixSender::start() {
	/* unimplemented, we can't be paused - TODO: or should we? */
}

/**
 * Temporarily pauses sending messages
 */
void IpfixSender::stop() {
	/* unimplemented, we can't be paused - TODO: or should we? */
}

/**
 * Add another IPFIX collector to export the stream to
 * the lowlevel stuff in handled by underlying ipfixlolib
 * @param ips handle to the Exporter
 * @param ip string of the IP
 * @param port port number
 * FIXME: support for other than UDP
 */
int IpfixSender::addCollector(const char *ip, uint16_t port, ipfix_transport_protocol proto)
{
	ipfix_exporter *ex = (ipfix_exporter *)ipfixExporter;

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
	}

	if(ipfix_add_collector(ex, ip, port, proto) != 0) {
		msg(MSG_FATAL, "IpfixSender: ipfix_add_collector of %s:%d failed", ip, port);
		return -1;
	}
	
	return 0;
}

/**
 * Announces a new Template
 * @param sourceID ignored
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
int IpfixSender::onDataTemplate(IpfixRecord::SourceID* sourceID, IpfixRecord::DataTemplateInfo* dataTemplateInfo)
{
	uint16_t my_template_id;
	uint16_t my_preceding;
	ipfix_exporter* exporter = (ipfix_exporter*)ipfixExporter;
	if (!exporter) {
		msg(MSG_ERROR, "sndIpfix: Exporter not set");
		return -1;
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
		msg(MSG_FATAL, "sndIpfix: ipfix_start_datatemplate_set failed");
		return -1;
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
		msg(MSG_FATAL, "sndIpfix: ipfix_put_template_data failed");
		free(data);
		return -1;
	}
	free(data);

	if (0 != ipfix_end_template_set(exporter, my_template_id)) {
		msg(MSG_FATAL, "sndIpfix: ipfix_end_template_set failed");
		return -1;
	}

	msg(MSG_INFO, "sndIpfix created template with ID %u", my_template_id);

	return 0;
}

/**
 * Invalidates a template; Does NOT free dataTemplateInfo
 * @param sourceID ignored
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
int IpfixSender::onDataTemplateDestruction(IpfixRecord::SourceID* sourceID, IpfixRecord::DataTemplateInfo* dataTemplateInfo)
{
	ipfix_exporter* exporter = (ipfix_exporter*)ipfixExporter;

	if (!exporter) {
		msg(MSG_ERROR, "sndIpfix: Exporter not set");
		return -1;
	}

	uint16_t my_template_id = dataTemplateInfo->templateId;


	/* Remove template from ipfixlolib */
	if (0 != ipfix_remove_template_set(exporter, my_template_id)) {
		msg(MSG_FATAL, "sndIpfix: ipfix_remove_template_set failed");
	}
	else
	{
		msg(MSG_INFO, "sndIpfix removed template with ID %u", my_template_id);
	}

	free(dataTemplateInfo->userData);

	return 0;
}


/**
 * Start generating a new Data Set unless the template ID is the same as the current Data Set.
 * Unfinished Data Set are terminated and sent if necessary.
 * @param templateId of the new Data Set
 * @return returns -1 on error, 0 otherwise
 */
int IpfixSender::startDataSet(uint16_t templateId) 
{
	ipfix_exporter* exporter = (ipfix_exporter*)ipfixExporter;
	uint16_t my_n_template_id = htons(templateId);
	
	/* check if we can use the current Data Set */
	//TODO: make maximum number of records per Data Set variable
	if((recordsInDataSet < 10) && (templateId == currentTemplateId))
		return 0;

	if(recordsInDataSet > 0)
		if(endAndSendDataSet() != 0)
			return -1;
	
	if (ipfix_start_data_set(exporter, my_n_template_id) != 0 ) {
		msg(MSG_FATAL, "sndIpfix: ipfix_start_data_set failed!");
		return -1;
	}

	currentTemplateId = templateId;
	return 0;
}
	

/**
 * Terminates and sends current Data Set if available.
 * @return returns -1 on error, 0 otherwise
 */
int IpfixSender::endAndSendDataSet() 
{
	if(recordsInDataSet > 0) {
		ipfix_exporter* exporter = (ipfix_exporter*)ipfixExporter;
	
		if (ipfix_end_data_set(exporter, recordsInDataSet) != 0) {
			msg(MSG_FATAL, "sndIpfix: ipfix_end_data_set failed");
			return -1;
		}

		if (ipfix_send(exporter) != 0) {
			msg(MSG_FATAL, "sndIpfix: ipfix_send failed");
			return -1;
		}

		recordsToRelease.clear();

		recordsInDataSet = 0;
		currentTemplateId = 0;
	}
	
	return 0;
}
	

/**
 * Put new Data Record in outbound exporter queue
 * @param rec Data Data Record
 */
int IpfixSender::onDataDataRecord(boost::shared_ptr<IpfixDataDataRecord> rec)
{
	IpfixRecord::DataTemplateInfo* dataTemplateInfo = rec->dataTemplateInfo.get();
	IpfixRecord::Data* data = rec->data;

	ipfix_exporter* exporter = (ipfix_exporter*)ipfixExporter;

	if (!exporter) {
		msg(MSG_ERROR, "sndIpfix: Exporter not set");
		return -1;
	}

	if(startDataSet(dataTemplateInfo->templateId) != 0)
		return -1;

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
			ipfix_put_data_field(exporter, data + fi->offset, fi->type.length);
		}

	}

	recordsInDataSet++;

	sentRecords++;
	
	recordsToRelease.push_back(rec);
	
	return 0;
}

/**
 * Send pending Data Set
 */
int IpfixSender::onIdle() 
{
	return endAndSendDataSet();
}


/**
 * Called by the logger timer thread. Dumps info using msg_stat
 */
void IpfixSender::stats()
{
	msg_stat("Concentrator: IpfixSender: %6d records sent", sentRecords);
	sentRecords = 0;
}

void IpfixSender::flowSinkProcess()
{
	msg(MSG_INFO, "Sink: now running FlowSink thread");
	while(!exitFlag) {
		boost::shared_ptr<IpfixRecord> ipfixRecord;
		if (!ipfixRecords.pop(1000, &ipfixRecord))
		{
			onIdle();
			continue;
		}
		{
			IpfixDataRecord* rec = dynamic_cast<IpfixDataRecord*>(ipfixRecord.get());
			if (rec) onDataRecord(rec->sourceID.get(), rec->templateInfo.get(), rec->dataLength, rec->data);
		}
		{
			boost::shared_ptr<IpfixDataDataRecord> rec = boost::dynamic_pointer_cast<IpfixDataDataRecord, IpfixRecord>(ipfixRecord);
			if (rec) onDataDataRecord(rec);
		}
		{
			IpfixOptionsRecord* rec = dynamic_cast<IpfixOptionsRecord*>(ipfixRecord.get());
			if (rec) onOptionsRecord(rec->sourceID.get(), rec->optionsTemplateInfo.get(), rec->dataLength, rec->data);
		}
		{
			IpfixTemplateRecord* rec = dynamic_cast<IpfixTemplateRecord*>(ipfixRecord.get());
			if (rec) onTemplate(rec->sourceID.get(), rec->templateInfo.get());
		}
		{
			IpfixDataTemplateRecord* rec = dynamic_cast<IpfixDataTemplateRecord*>(ipfixRecord.get());
			if (rec) onDataTemplate(rec->sourceID.get(), rec->dataTemplateInfo.get());
		}
		{
			IpfixOptionsTemplateRecord* rec = dynamic_cast<IpfixOptionsTemplateRecord*>(ipfixRecord.get());
			if (rec) onOptionsTemplate(rec->sourceID.get(), rec->optionsTemplateInfo.get());
		}
		{
			IpfixTemplateDestructionRecord* rec = dynamic_cast<IpfixTemplateDestructionRecord*>(ipfixRecord.get());
			if (rec) onTemplateDestruction(rec->sourceID.get(), rec->templateInfo.get());
		}
		{
			IpfixDataTemplateDestructionRecord* rec = dynamic_cast<IpfixDataTemplateDestructionRecord*>(ipfixRecord.get());
			if (rec) onDataTemplateDestruction(rec->sourceID.get(), rec->dataTemplateInfo.get());
		}
		{
			IpfixOptionsTemplateDestructionRecord* rec = dynamic_cast<IpfixOptionsTemplateDestructionRecord*>(ipfixRecord.get());
			if (rec) onOptionsTemplateDestruction(rec->sourceID.get(), rec->optionsTemplateInfo.get());
		}
	}
}
