/*
 * PSAMP Reference Implementation
 *
 * Implementation of an IPFIX exporter packet sink
 * using Jan Petranek's ipfixlolib
 *
 * Author: Michael Drueing <michael@drueing.de>
 *         Gerhard Muenz <gerhard.muenz@gmx.de>
 *
 */

#ifndef EXPORTER_SINK_H
#define EXPORTER_SINK_H

#include "common/ipfixlolib/ipfixlolib.h"
#include "common/ipfixlolib/ipfix.h"
#include "common/ConcurrentQueue.h"
#include "common/msg.h"

#include "common/Thread.h"
#include "modules/packet/Template.h"
#include "modules/packet/Packet.h"

#include "core/Module.h"
#include "core/Notifiable.h"

// the maximum number of packets to be queued
#define MAX_PACKETS 1024
// the default maximum of records per IPFIX packet sent
#define MAX_RECORDS_PER_PACKET 10
// maximum time a packet may be in the exporter. in milliseconds.
#define MAX_PACKET_LIFETIME 400

class PSAMPExporterModule
	: public Module, public Source<NullEmitable*>, public Destination<Packet*>, public Notifiable
{
public:
        PSAMPExporterModule(Template *tmpl, uint32_t observationDomainId);

	~PSAMPExporterModule();

        virtual void receive(Packet* packet);

        virtual void onTimeout(void* dataPtr);
        
        virtual void performStart();

        virtual void performShutdown();
        
        /* max records per IPFIX packet */
        inline bool setMaxRecords(int x)
        {
                ipfix_maxrecords=x;
                return true;
        }

        inline int getMaxRecords()
        {
                return ipfix_maxrecords;
	}

	inline bool setExportTimeout(int ms)
	{
		exportTimeout=ms;
                return true;
	}

        bool addCollector(const char *address, uint16_t port, ipfix_transport_protocol protocol);

private:

        // the usage of the following functions is:
        //
        // StartPacketStream();
        // while (more_packets AND timeout_not_met)
        // {
        //   AddPaket(getNextPacket());
        // }
        // FlushPacketStream();

        // start a new IPFIX packet stream
        void startNewPacketStream();

        // Add this packet to the packet stream
	// returns true, if the Packet was successfully added
        bool addPacket(Packet *pck);

        // send out the IPFIX packet stream and reset
        void flushPacketStream();

	uint32_t sourceID;
        Template *templ;

        ipfix_exporter *exporter;

        // these packets need to be release()'d after we send the current IPFIX packet
        int numPacketsToRelease;
        Packet *packetsToRelease[MAX_PACKETS];

        // put this many packets into one big IPFIX packet
	int ipfix_maxrecords;

        // time-constraint for exporting data, in ms
	int exportTimeout;

	// counter for the number of packets we need to send
	int pckCount;
	
	// timer stamp; this value is used to see if wee need to do a flush based on the timeout
	uintptr_t timerFlag;
};

#endif
