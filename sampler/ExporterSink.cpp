/*
 * PSAMP Reference Implementation
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

#include "msg.h"
#include "ipfixlolib/ipfixlolib.h"

#include "ExporterSink.h"

using namespace std;


// the usage of the following functions is:
//
// StartPacketStream();
// while (more_packets AND timeout_not_met)
// {
//   AddPaket(getNextPacket());
// }
// FlushPacketStream();
//
// This schema is realized in exporterSinkProcess

// start a new IPFIX packet stream
void ExporterSink::startNewPacketStream()
{
    unsigned short net_tmplid = htons(templ->getTemplateID());
    DPRINTF("Starting new packet stream\n");
    ipfix_start_data_set(exporter, net_tmplid);
}

// Add this packet to the packet stream
// returns true, if the Packet was successfully added
bool ExporterSink::addPacket(Packet *pck)
{
    unsigned short ttype, tlength, toffset, theader;
    unsigned short enc_length = 0;
    void *data, *metadata;
    uint8_t *enc_value = 0;
    bool ret = true;

    // first check, if we can buffer this packet
    if(!(numPacketsToRelease < MAX_PACKETS)) 
    {
	msg(MSG_ERROR, "ExporterSink: packet buffer too small, packet dropped.");
	pck->release();
	return false;
    }

    
    // now, check if packet matches template requirements, i.e. if all fields are available
    if(templ->checkPacketConformity(pck->classification)) 
    {
	// store current position in order to go back in case of problems
	ipfix_set_data_field_marker(exporter);

	for(int i = 0; i < templ->getFieldCount(); i++) 
	{
	    templ->getFieldInfo(i, &ttype, &tlength, &toffset, &theader);
	    if(ttype >= 0x8000) 
	    {
		// it is a meta-field --> get the metadata
		metadata = templ->getMetaFieldData(i);
		ipfix_put_data_field(exporter, metadata, tlength);
		metaFieldsToRelease[numMetaFieldsToRelease++] = metadata;
	    }
	    else if(ttype == IPFIX_TYPEID_flowStartSeconds)
	    {
		ipfix_put_data_field(exporter, &(pck->time_sec_nbo), tlength);
	    }
	    else if(ttype == IPFIX_TYPEID_flowStartMilliSeconds)
	    {
		ipfix_put_data_field(exporter, &(pck->time_msec_ipfix), tlength);
	    }
	    else if(ttype == IPFIX_TYPEID_flowStartMicroSeconds)
	    {
		ipfix_put_data_field(exporter, &(pck->time_usec_nbo), tlength);
	    }
	    else if(tlength == 65535)
	    {
		// variable length field
		if((data = pck->getVariableLengthPacketData(&tlength, &enc_value, &enc_length, toffset, theader)) != NULL) 
		{
		    // put the length information first
		    ipfix_put_data_field(exporter, enc_value, enc_length);
		    ipfix_put_data_field(exporter, data, tlength);
		}
		else
		{
		    msg(MSG_ERROR, "ExporterSink: getVariableLengthPacketData returned NULL! This should never happen!");
		    // delete the fields that we have already added
		    ipfix_delete_data_fields_upto_marker(exporter);
		    ret = false;
		    break;
		}
	    }
	    else
	    {
		// check if getPacketData actually returns data
		// Note: getPacketData checks if data of size tlength is available.
		//       if not, it returns NULL
		if( (data = pck->getPacketData(toffset, theader, tlength)) != NULL) {
		    ipfix_put_data_field(exporter, data, tlength);
		} else {
		    msg(MSG_ERROR, "ExporterSink: getPacketData returned NULL! packet length or pcap capture length is too small.");
		    // delete the fields that we have already added
		    ipfix_delete_data_fields_upto_marker(exporter);
		    ret = false;
		    break;
		}
	    }
	}
    }
    else
    {
	DPRINTF("Packet does not contain all fields required by the template! Skip this packet.\n");
	ret = false;
    }

    // if we will export the packet, we keep and and release it later, after we have sent the data
    if(ret == true)
    {
	DPRINTF("Adding packet to buffer\n");
	packetsToRelease[numPacketsToRelease++] = pck;
    }
    else
    {
	// we do no export this packet, i.e. we can release it right now.
	DPRINTF("Releasing packet\n");
	pck->release();
    }

    return ret;
}

// send out the IPFIX packet stream and reset
void ExporterSink::flushPacketStream()
{
    // end the packet stream and send the IPFIX packet out through the wire
    ipfix_end_data_set(exporter);
    ipfix_send(exporter);

    DPRINTF("Flushing %d packets from buffer\n", numPacketsToRelease);
    for(int i = 0; i < numPacketsToRelease; i++) {
	(packetsToRelease[i])->release();
    }
    // now release the additional metadata fields
    DPRINTF("Flushing %d Metadata fields from buffer\n", numMetaFieldsToRelease);
    for(int i = 0; i < numMetaFieldsToRelease; i++) {
	free(metaFieldsToRelease[i]);
    }
    numPacketsToRelease = 0;
    numMetaFieldsToRelease = 0;
}


void *ExporterSink::exporterSinkProcess(void *arg)
{
	ExporterSink *sink = (ExporterSink *)arg;
	ConcurrentQueue<Packet*> *queue = sink->getQueue();
	Packet *p;
	bool result;
	// our deadline
	struct timeval deadline;
	int pckCount;

	msg(MSG_INFO, "Sink: now running ExporterSink thread");

	while(!sink->exitFlag) {
		sink->startNewPacketStream();

		// let's get the first packet
		gettimeofday(&deadline, 0);
		deadline.tv_sec++; // let's wait one second before checking exitFlag again
		do
		{
		    result = queue->popAbs(deadline, &p);
		    if(sink->exitFlag)
			break;
		    if(result == true)
			// we got a packet, so let's add the record
			result = sink->addPacket(p);
		}
		while(result == false);
			
		pckCount = 1;

		// now calculate the deadline by which the packet has to leave the exporter
		gettimeofday(&deadline, 0);
		deadline.tv_usec += sink->exportTimeout * 1000L;
		if(deadline.tv_usec > 1000000L) {
			deadline.tv_sec += (deadline.tv_usec / 1000000L);
			deadline.tv_usec %= 1000000L;
		}

		while(!sink->exitFlag && (pckCount < sink->ipfix_maxrecords)) {
			// Try to get next packet from queue before our deadline
			result = queue->popAbs(deadline, &p);

			// check for timeout and break loop if neccessary
			if(!result)
				break;

			// no timeout received, continue waiting, but

			// count only if packet was added
			if(sink->addPacket(p) == true)
			    pckCount++;
		}
		sink->flushPacketStream();
	}
	return 0;
}


bool ExporterSink::addCollector(const char *address, unsigned short port, const char *protocol)
{
	ipfix_transport_protocol proto;

	if(strcasecmp(protocol, "TCP") == 0) {
		proto = TCP;
	} else if(strcasecmp(protocol, "UDP") == 0) {
		proto = UDP;
	} else {
		msg(MSG_ERROR, "ExporterSink: invalid protocol %s for %s",
		    protocol, address);
		return false;
	}

	DPRINTF("Adding %s://%s:%d\n", protocol, address, port);
	return(ipfix_add_collector(exporter, address, port, proto) == 0);
}
