/*
 * PSAMP Reference Implementation
 *
 * ExporterSink.h
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
#include "msg.h"

#include "Thread.h"
#include "Template.h"
#include "Packet.h"
#include "Sink.h"

// the maximum number of packets to be queued
#define MAX_PACKETS 1024
// the default maximum of IPFIX packet per big IPFIX packet sent
#define IPFIX_PACKETS_MAX 10
// maximum time a packet may be in the exporter. in milliseconds.
#define MAX_PACKET_LIFETIME 400

class ExporterSink : public Sink
{
public:
        ExporterSink(Template *tmpl, int sID) : sourceID(sID),
                templ(tmpl), thread(ExporterSink::exporterSinkProcess),
		numPacketsToRelease(0), numMetaFieldsToRelease(0), 
		ipfix_maxpackets(IPFIX_PACKETS_MAX), exportTimeout(MAX_PACKET_LIFETIME), 
		exitFlag(false)
        {
                int ret, i, tmplid;
                unsigned short ttype, tlength, toffset, theader;

                // generate the exporter
                ret = ipfix_init_exporter(sourceID, &exporter);
                if(ret) {
                        msg(MSG_FATAL, "ExporterSink: error initializing IPFIX exporter");
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


	~ExporterSink()
        {
                ipfix_deinit_exporter(exporter);
        };

        bool runSink()
        {
                msg(MSG_DEBUG, "Sink: now starting ExporterSink thread");
                return (thread.run(this));
        };

        bool terminateSink()
        {
                exitFlag = true;
		return true;
        };

        // the usage of the following functions is:
        //
        // StartPacketStream();
        // while (more_packets AND timeout_not_met)
        // {
        //   AddPaket(getNextPacket());
        // }
        // FlushPacketStream();

        // start a new IPFIX packet stream
        void startNewPacketStream()
        {
                unsigned short net_tmplid = htons(templ->getTemplateID());
                DPRINTF("Starting new packet stream\n");
                numPacketsToRelease = 0;
                ipfix_start_data_set(exporter, net_tmplid);
        }

        // Add this packet to the packet stream
        void addPacket(Packet *pck)
        {
                unsigned short ttype, tlength, toffset, theader;
		void *data, *metadata;
                // first, store the packet to be released later, after we have sent the data
                DPRINTF("Adding packet to stream\n");
                packetsToRelease[numPacketsToRelease++] = pck;

		// first check if packet matches template requirements, i.e. if all fields are available
		if(templ->checkPacketConformity(pck->classification)) 
		{
		    // store current position in order to go back in case of problems
		    ipfix_set_data_field_marker(exporter);

		    for(int i = 0; i < templ->getFieldCount(); i++) 
		    {
			templ->getFieldInfo(i, &ttype, &tlength, &toffset, &theader);
			if (ttype > 0x8000) {
			    // it is a meta-field --> get the metadata
			    metadata = templ->getMetaFieldData(i);
			    ipfix_put_data_field(exporter, metadata, tlength);
			    metaFieldsToRelease[numMetaFieldsToRelease++] = metadata;
			} else {
			    // check if getPacketData actually returns data
			    // Note: getPacketData checks if data of size tlength is available.
			    //       if not, it returns NULL
			    if( (data = pck->getPacketData(toffset, theader, tlength)) != NULL) {
				ipfix_put_data_field(exporter, data, tlength);
			    } else {
				msg(MSG_ERROR, "ExporterSink: getPacketData returned NULL! capturelen is too small.");
				// delete the fields that we have already added
				ipfix_delete_data_fields_upto_marker(exporter);
			    }
			}
		    }
		}
		else
		{
		    DPRINTF("Packet does not contain all fields required by the template! Skip this packet.\n");
		    ipfix_delete_data_fields_upto_marker(exporter);
		}
        }

        // send out the IPFIX packet stream and reset
        void flushPacketStream()
        {
                // end the packet stream and send the IPFIX packet out through the wire
                ipfix_end_data_set(exporter);
                ipfix_send(exporter);

                DPRINTF("Flushing %d packets from stream\n", numPacketsToRelease);
                for(int i = 0; i < numPacketsToRelease; i++) {
                        (packetsToRelease[i])->release();
                }
		// now release the additional metadata fields
		DPRINTF("Flushing %d Metadata fields from stream\n", numMetaFieldsToRelease);
		for(int i = 0; i < numMetaFieldsToRelease; i++) {
			free(metaFieldsToRelease[i]);
		}
                numPacketsToRelease = 0;
		numMetaFieldsToRelease = 0;
        }


        /* max packets per big IPFIX packet */
        bool setMaxIpfixPP(int x)
        {
                ipfix_maxpackets=x;

                return true;
        }

        int getMaxIpfixPP()
        {
                return ipfix_maxpackets;
	}

	bool setExportTimeout(int ms)
	{
		exportTimeout=ms;

                return true;
	}

        bool addCollector(char *address, unsigned short port, const char *protocol);

protected:
        int sourceID;
        Template *templ;
        Thread thread;
        static void *exporterSinkProcess(void *);

        ipfix_exporter *exporter;

        // these packets need to be release()'d after we send the current IPFIX packet
        int numPacketsToRelease;
        Packet *packetsToRelease[MAX_PACKETS];
	
	int numMetaFieldsToRelease;
	void *metaFieldsToRelease[MAX_PACKETS*10];

        // put this many packets into one big IPFIX packet
	int ipfix_maxpackets;

        // time-constraint for exporting data, in ms
	int exportTimeout;

public:
        bool exitFlag;
};

#endif
