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

#include "ipfixlolib/ipfixlolib.h"
#include "concentrator/ipfix.hpp"
#include "common/ConcurrentQueue.h"
#include "common/msg.h"

#include "common/Thread.h"
#include "sampler/Template.h"
#include "Packet.h"

#include "reconf/Module.h"
#include "reconf/Notifiable.h"

// the maximum number of packets to be queued
#define MAX_PACKETS 1024
// the default maximum of records per IPFIX packet sent
#define MAX_RECORDS_PER_PACKET 10
// maximum time a packet may be in the exporter. in milliseconds.
#define MAX_PACKET_LIFETIME 400

class PSAMPExporterModule
	: public Module, public Source<Packet*>, public Destination<Packet*>, public Notifiable
{
public:
        PSAMPExporterModule(Template *tmpl, int sID)
			: sourceID(sID), templ(tmpl), thread(process),
			  exporter(NULL),
			  numPacketsToRelease(0), numMetaFieldsToRelease(0),
			  ipfix_maxrecords(MAX_RECORDS_PER_PACKET),
			  exportTimeout(MAX_PACKET_LIFETIME)
        {
		int ret, i, tmplid;
		unsigned short ttype, tlength, toffset, theader;

		// generate the exporter
		ret = ipfix_init_exporter(sourceID, &exporter);
		if (ret) {
			msg(MSG_FATAL, "error initializing IPFIX exporter");
			exit(1);
		}

                // generate the ipfix template
                tmplid = templ->getTemplateID();
                ret =  ipfix_start_template_set(exporter, tmplid, templ->getFieldCount());

                for(i = 0; i < templ->getFieldCount(); i++) {
			templ->getFieldInfo(i, &ttype, &tlength, &toffset, &theader);
			ipfix_put_template_field(exporter, tmplid, ttype, tlength, 0);
                }

                ipfix_end_template_set(exporter, tmplid);
        };


	~PSAMPExporterModule()
        {
		shutdown(false); // try to shutdown; no error if not running
                ipfix_deinit_exporter(exporter);

                // free the remaining packets
                for (int i = 0; i < numPacketsToRelease; i++) {
			(packetsToRelease[i])->removeReference();
		}
        };

        virtual void receive(Packet* packet)
        {
		queue.push(packet);
        }

        virtual void onTimeout(uint32_t flag)
        {
        	
        }
        
        virtual void performStart()
        {
                msg(MSG_DEBUG, "Sink: now starting PSAMPExporter thread");
                thread.run(this);
        }

        virtual void performShutdown()
        {
                msg(MSG_DEBUG, "Sink: waiting for exporter thread");
                connected.shutdown();
                queue.notifyShutdown();
                thread.join();
                msg(MSG_DEBUG, "Sink: exporter thread joined");
        }

        /* max records per IPFIX packet */
        bool setMaxRecords(int x)
        {
                ipfix_maxrecords=x;
                return true;
        }

        int getMaxRecords()
        {
                return ipfix_maxrecords;
	}

	bool setExportTimeout(int ms)
	{
		exportTimeout=ms;
                return true;
	}

        bool addCollector(const char *address, unsigned short port, const char *protocol);

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

        static void* process(void *);


	int sourceID;
        Template *templ;
        Thread thread;
        ConcurrentQueue<Packet*> queue;

        ipfix_exporter *exporter;

        // these packets need to be release()'d after we send the current IPFIX packet
        int numPacketsToRelease;
        Packet *packetsToRelease[MAX_PACKETS];

	int numMetaFieldsToRelease;
	void *metaFieldsToRelease[MAX_PACKETS*10];

        // put this many packets into one big IPFIX packet
	int ipfix_maxrecords;

        // time-constraint for exporting data, in ms
	int exportTimeout;
};

#endif
