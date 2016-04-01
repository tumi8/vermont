/*
 * PSAMP Reference Implementation
 * Copyright (C) 2014 Oliver Gasser
 *
 * ExporterSink.cpp
 *
 * Implementation of an IPFIX exporter packet sink
 * using Jan Petranek's ipfixlolib
 *
 * Changed by Gerhard Muenz
 *   return value of addPacket evaluated
 *
 * Author: Michael Drueing <michael@drueing.de>
 *
 */

#include <sys/time.h>
#include <time.h>

#include "common/msg.h"
#include "common/ipfixlolib/ipfixlolib.h"

#include "PSAMPExporterModule.h"
#include "core/Timer.h"

using namespace std;

PSAMPExporterModule::PSAMPExporterModule(Template *tmpl, uint32_t observationDomainId)
		: sourceID(observationDomainId), templ(tmpl),
		  exporter(NULL),
		  numPacketsToRelease(0),
		  ipfix_maxrecords(MAX_RECORDS_PER_PACKET),
		  exportTimeout(MAX_PACKET_LIFETIME), pckCount(0), timerFlag(0)
{
	int ret, i, tmplid;
	uint16_t offset, header;
	InformationElement::IeInfo ie;

	// generate the exporter
	ret = ipfix_init_exporter(IPFIX_PROTOCOL, sourceID, &exporter);
	if (ret) {
		msg(MSG_FATAL, "error initializing IPFIX exporter");
		exit(1);
	}

    // generate the ipfix template
    tmplid = templ->getTemplateID();
    ret =  ipfix_start_template(exporter, tmplid, templ->getFieldCount());

    for(i = 0; i < templ->getFieldCount(); i++) {
		templ->getFieldInfo(i, &ie, &offset, &header);
		ipfix_put_template_field(exporter, tmplid, ie.id, ie.length, ie.enterprise);
    }

    ipfix_end_template(exporter, tmplid);

	// start new Data Set
	startNewPacketStream();
}

PSAMPExporterModule::~PSAMPExporterModule()
{
        ipfix_deinit_exporter(&exporter);

        // free the remaining packets
        for (int i = 0; i < numPacketsToRelease; i++) {
		(packetsToRelease[i])->removeReference();
	}
}


void PSAMPExporterModule::startNewPacketStream()
{
    unsigned short net_tmplid = htons(templ->getTemplateID());
    DPRINTF("Starting new PSAMP packet");
    ipfix_start_data_set(exporter, net_tmplid);
}

// Add this packet to the packet stream
// returns true, if the Packet was successfully added
bool PSAMPExporterModule::addPacket(Packet *pck)
{
	InformationElement::IeInfo ie;
	uint16_t length, offset, header;
	uint8_t enc_length = 0;
	void *data;
	uint8_t *enc_value = 0;

	// first check, if we can buffer this packet
	if (!(numPacketsToRelease < MAX_PACKETS)) {
		msg(MSG_ERROR, "packet buffer too small, packet dropped.");
		DPRINTF("dropping packet");
		pck->removeReference();
		return false;
	}

	// now, check if packet matches template requirements, i.e. if all fields are available
	if (!templ->checkPacketConformity(pck->classification)) {
		DPRINTF("Packet does not contain all fields required by the template! Skip this packet.");
		goto error2;
	}

	// store current position in order to go back in case of problems
	ipfix_set_data_field_marker(exporter);

	for (int i = 0; i < templ->getFieldCount(); i++) {
		templ->getFieldInfo(i, &ie, &offset, &header);
		if (ie.enterprise == 0 && (ie.id == IPFIX_TYPEID_flowStartSeconds || ie.id == PSAMP_TYPEID_observationTimeSeconds)) {
			ipfix_put_data_field(exporter, &(pck->time_sec_nbo), ie.length);
		} else if (ie.enterprise == 0 && (ie.id == IPFIX_TYPEID_flowStartMilliseconds || ie.id == PSAMP_TYPEID_observationTimeMilliseconds)) {
			ipfix_put_data_field(exporter, &(pck->time_msec_nbo), ie.length);
		} else if (ie.enterprise == 0 && (ie.id == IPFIX_TYPEID_flowStartMicroseconds || ie.id == PSAMP_TYPEID_observationTimeMicroseconds)) {
			ipfix_put_data_field(exporter, &(pck->time_usec_nbo), ie.length);
		} else if (ie.length == 65535) {
			// variable length field
			data = pck->getVariableLengthPacketData(&length, &enc_value, &enc_length, offset, header);
			if(data == NULL) {
				msg(MSG_ERROR, "ExporterSink: getVariableLengthPacketData returned NULL! This should never happen!");
				goto error1;
			}
			// put the length information first
			ipfix_put_data_field(exporter, enc_value, enc_length);
			ipfix_put_data_field(exporter, data, length);
		} else {
			// check if getPacketData actually returns data
			// Note: getPacketData checks if data of size ie.length is available.
			//       if not, it returns NULL
			data = pck->getPacketData(offset, header, ie.length);
			if(data == NULL) {
				msg(MSG_ERROR, "ExporterSink: getPacketData returned NULL! packet length or pcap capture length is too small.");
				goto error1;
			}
			ipfix_put_data_field(exporter, data, ie.length);
		}
	}

	// if we will export the packet, we keep and and release it later, after we have sent the data
	DPRINTF("Adding packet to buffer");
	packetsToRelease[numPacketsToRelease++] = pck;
	return true;

error1:
	// delete the fields that we have already added
	ipfix_delete_data_fields_upto_marker(exporter);

error2:
	// we do no export this packet, i.e. we can release it right now.
	DPRINTF("dropping packet");
	pck->removeReference();
	return false;
}

// send out the IPFIX packet stream and reset
void PSAMPExporterModule::flushPacketStream() {
	// do nothing if no records can be sent
	if (numPacketsToRelease == 0) {
		return;
	}

	// end the packet stream and send the IPFIX packet out through the wire
	ipfix_end_data_set(exporter, numPacketsToRelease);
	ipfix_send(exporter);

	DPRINTF("dropping %d packets", numPacketsToRelease);
	for (int i = 0; i < numPacketsToRelease; i++) {
		(packetsToRelease[i])->removeReference();
	}

	numPacketsToRelease = 0;
	pckCount = 0;

	// start new Data Set
	startNewPacketStream();
}

bool PSAMPExporterModule::addCollector(const char *address, uint16_t port, ipfix_transport_protocol protocol)
{
	DPRINTF("Adding %i://%s:%d", protocol, address, port);
	return(ipfix_add_collector(exporter, address, port, protocol, NULL) == 0);
}

void PSAMPExporterModule::receive(Packet* p)
{
	if (pckCount == 0) {
		if (!addPacket(p))
			return;

		pckCount = 1;

		// now calculate the deadline by which the packet has to leave the exporter
		struct timespec deadline;
		addToCurTime(&deadline, exportTimeout);

		if (timer)
			timer->addTimeout(this, deadline, (void*)++timerFlag);
	} else {
		// count only if packet was added
		if (!addPacket(p))
			return;
		pckCount++;
	}

	if (pckCount == ipfix_maxrecords)
		flushPacketStream();
}

void PSAMPExporterModule::onTimeout(void* flag)
{
	// only if the timer event wasn't processed yet we need to
	// flush the packet stream.
	if ((uintptr_t)flag == timerFlag)
		flushPacketStream();
}

void PSAMPExporterModule::performStart()
{

}

void PSAMPExporterModule::performShutdown()
{
	// we could destroy the template here
}
