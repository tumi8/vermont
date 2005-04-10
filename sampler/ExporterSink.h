/*
 * PSAMP Reference Implementation
 *
 * ExporterSink.h
 *
 * Implementation of an IPFIX exporter packet sink
 * using Jan Petranek's ipfixlolib
 *
 * Author: Michael Drueing <michael@drueing.de>
 *
 */
 
#ifndef EXPORTER_SINK_H
#define EXPORTER_SINK_H

#include "PacketReceiver.h"
#include "Thread.h"
#include "Template.h"
extern "C" {
#include "ipfixlolib.h"
}

// the maximum number of packets to be queued
#define MAX_PACKETS 1024
// the default maximum of IPFIX packet per big IPFIX packet sent
#define IPFIX_PACKETS_MAX 10

class ExporterSink : public PacketReceiver
{
public:
    ExporterSink(Template *tmpl, int sID) : sourceID(sID),
        templ(tmpl), thread(ExporterSink::exporterSinkProcess), exitFlag(false),
        numPacketsToRelease(0), ipfix_maxpackets(IPFIX_PACKETS_MAX)
  {
    int ret, i, tmplid;
    unsigned short ttype, tlength, toffset;
    
    // generate the exporter
    ret = ipfix_init_exporter(sourceID, &exporter);
    if (ret)
    {
      LOG("Error initializing IPFIX exporter\n");
      exit(1);
    }
    
    // generate the ipfix template
    tmplid = templ->getTemplateID();
    ret =  ipfix_start_template_set(exporter, tmplid, templ->getFieldCount());
    for (i = 0; i < templ->getFieldCount(); i++)
    {
      templ->getFieldInfo(i, &ttype, &tlength, &toffset);
      ipfix_put_template_field(exporter, tmplid, tlength, ttype, 0);
    }
    ipfix_end_template_set(exporter, tmplid);
  };
  
  void AddCollector(char *address, unsigned short port, const char *protocol)
  {
    ipfix_transport_protocol proto;
    
    if (strcasecmp(protocol, "TCP") == 0)
      proto = TCP;
    else if (strcasecmp(protocol, "UDP") == 0)
      proto = UDP;
    else
      LOG("Invalid protocol %s\n", protocol);
      
    LOG("Adding %s://%s:%d\n", protocol, address, port);
    ipfix_add_collector(exporter, address, port, proto);
  }

  ~ExporterSink()
  {
    ipfix_deinit_exporter(exporter);
  };

  inline void runSink()
  {
    thread.run(this);
  };

  inline void terminateSink()
  {
    exitFlag = true;
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
  void StartNewPacketStream()
  {
    unsigned short net_tmplid = htons(templ->getTemplateID());
    LOG("Starting new packet stream\n");
    numPacketsToRelease = 0;
    ipfix_start_data_set(exporter, &net_tmplid);
  }
  
  // Add this packet to the packet stream
  void AddPacket(Packet *pck)
  {
    unsigned short ttype, tlength, toffset;
    // first, store the packet to be released later, after we have sent the data
    LOG("Adding packet to stream\n");
    packetsToRelease[numPacketsToRelease++] = pck;
    
    for (int i = 0; i < templ->getFieldCount(); i++)
    {
      templ->getFieldInfo(i, &ttype, &tlength, &toffset);
      ipfix_put_data_field(exporter, pck->getPacketData(toffset), tlength);
    }
  }
  
  // send out the IPFIX packet stream and reset
  void FlushPacketStream()
  {
    // end the packet stream and send the IPFIX packet out through the wire
    ipfix_end_data_set(exporter);
    ipfix_send(exporter);
    
    LOG("Flushing %d packets from stream\n", numPacketsToRelease);
    for (int i = 0; i < numPacketsToRelease; i++)
    {
      (packetsToRelease[i])->release();
    }
    numPacketsToRelease = 0;
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

protected:
  int sourceID;
  Template *templ;
  Thread thread;
  static void *exporterSinkProcess(void *);
  
  ipfix_exporter *exporter;

  // these packets need to be release()'d after we send the current IPFIX packet
  int numPacketsToRelease;
  Packet *packetsToRelease[MAX_PACKETS];

  int ipfix_maxpackets;

public:
  bool exitFlag;
};

#endif
