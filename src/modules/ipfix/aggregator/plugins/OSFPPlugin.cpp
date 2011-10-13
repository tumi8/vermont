/*
 * Vermont Aggregator Subsystem
 * Copyright (C) 2009 Vermont Project
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
#ifdef PLUGIN_SUPPORT_ENABLED
#include "OSFPPlugin.h"
#include "common/msg.h"
#include <arpa/inet.h>
#include <features.h>
#include "modules/ipfix/aggregator/PacketHashtable.h"
#include "modules/ipfix/aggregator/plugins/PacketFunctions.h"


#if defined(__FreeBSD__) || defined(__APPLE__)
// FreeBSD does not have a struct iphdr. Furthermore, the struct members in
// struct udphdr and tcphdr differ from the linux version. We therefore need
// to define them
#include <osdep/freebsd/iphdrs.h>
#else
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#endif

using namespace std;

#define TCP_HEADER_WITHOUT_OPTIONS 20

OSFPPlugin::OSFPPlugin(){
    OSFPPlugin(0, (char*) "dump.csv");
}

OSFPPlugin::OSFPPlugin(const uint32_t maxPckts, std::string file){
    maxPackets = maxPckts;
    dumpFile = file;
    msg(MSG_INFO, "OSFPPlugin instantiated");
    msg(MSG_INFO, "  - dump file: %s", dumpFile.c_str());
    msg(MSG_INFO, "  - max connections: %i", maxPackets);
    filestream.open(dumpFile.c_str(), ios_base::out);
    filestream << "PCAP Timestamp : IP Source : TCP Source Port : IP Dest : TCP Dest Port : IP IHL : IP TOS : IP ID : "<<
              "IP Protocol : IP Dont Fragment : IP TTL : TCP Window Size : TCP MSS : TCP Window Scale : TCP SACK permitted Option : "<<
              "TCP NOP Option : TCP Timestamp set : TCP Timestamp : TCP Timestamp Secr : TCP Urgent Flag : "<<
              "TCP Push Flag : TCP Reset Flag : TCP FIN Flag : TCP Urgent Pointer : TCP EOL Option set : TCP Data Offset : " <<
              "TCP Sequence Number : TCP Acknowlegment Number : TCP Reserved 1 : TCP Reserved 2 : TCP Syn ACK None : LEN : " <<
              "Options Corrupt : Ordered Options" << endl;
}

OSFPPlugin::~OSFPPlugin(){
    filestream.close();
}


/**
 * gets called when a flow has been deleted
 */
void OSFPPlugin::flowDeleted(const HashtableBucket* bucket){
    map.erase(bucket->hash);
}


/**
 * gets called when a new flow has been generated
 */
void OSFPPlugin::newFlowReceived(const HashtableBucket* bucket){
    map[bucket->hash] = 0;
}


/**
 * gets called when a new packet has been received
 */
void OSFPPlugin::newPacketReceived(const Packet* p, uint32_t hash){
    // This plugin only works with TCP Packets
    if (p->ipProtocolType != Packet::TCP){
        return;
    }

    /* is packet tracking needed?
       if yes, check if not more than maxPackets */
    if (maxPackets > 0){
        if (map.find(hash) != map.end()){
            ++map[hash];
        }
        else{
            map[hash] = 1;
        }
        if (map[hash] > maxPackets){
            return;
        }
    }
    //start processing the packet
    processPacket(p);
}

void OSFPPlugin::processPacket(const Packet* p){
    struct TCPOptions options;
    /*initialize values*/
    memset(&options, 0, sizeof(struct TCPOptions));
    options.mss = 1460;
    unsigned char *doff;
    uint16_t doffval;
    const iphdr* ipheader;
    const tcphdr* tcpheader;
    OSFingerprint fingerprint;

    doff = p->transportHeader + 12;
    doffval = (uint16_t)((*doff) >> 4);

    if (doffval > 5){
        options.mss = 1500 - (doffval * 4 + sizeof(p->netHeader));
        options.has_options = 1;

        /*parse option fields:*/
        fingerprint.ordered_options = parseTCPOptions(options, p, doffval*4);

    }


    ipheader = (iphdr*) p->netHeader;
    tcpheader = (tcphdr*) p->transportHeader;

    fingerprint.m_Pcap_Timestamp = p->timestamp;
    fingerprint.m_TCP_Options = options;
    struct in_addr saddr;
    saddr.s_addr = ipheader->saddr;
    fingerprint.m_IP_Source = saddr;
    fingerprint.m_TCP_SourcePort = ntohs(tcpheader->source);
    struct in_addr daddr;
    daddr.s_addr = (in_addr_t) ipheader->daddr;
    fingerprint.m_IP_Dest = daddr;
    fingerprint.m_TCP_DestPort = ntohs(tcpheader->dest);
    fingerprint.m_IP_IHL = ipheader->ihl;
    fingerprint.m_IP_TOS = (uint16_t) ipheader->tos;
    fingerprint.m_IP_ID = ntohs(ipheader->id);
    fingerprint.m_IP_Protocol = (uint16_t) ipheader->protocol;
    fingerprint.m_IP_DF = (bool) ntohs(IP_DF);
    fingerprint.m_IP_TTL = fingerprint.guessInitialTTL(ipheader->ttl);
    fingerprint.m_TCP_Window_Size = ntohs(tcpheader->window);
    fingerprint.m_TCP_URG = (bool) tcpheader->urg;
    fingerprint.m_TCP_PSH = (bool) tcpheader->psh;
    fingerprint.m_TCP_RST = (bool) tcpheader->rst;
    fingerprint.m_TCP_FIN = (bool) tcpheader->fin;
    fingerprint.m_TCP_URG_PTR = (bool) tcpheader->urg_ptr;
    fingerprint.m_TCP_DOFF = ntohs(tcpheader->doff);
    fingerprint.m_TCP_SEQ = ntohl(tcpheader->seq);
    fingerprint.m_TCP_ACK_SEQ = ntohl(tcpheader->ack_seq);
    fingerprint.m_TCP_RES1 = ntohs(tcpheader->res1);
    fingerprint.m_TCP_RES2 = ntohs(tcpheader->res2);
    fingerprint.m_TCP_SYN = (bool)tcpheader->syn;
    fingerprint.m_TCP_ACK = (bool)tcpheader->ack;
    fingerprint.m_Data_Length = p->data_length;

    writeToFile(&fingerprint);
    fingerprint.detectOS();
    OSDetail detail = OSDetail(fingerprint.m_OS_Type, fingerprint.m_OS_Version, "", OSDetail::FINGERPRINT);
    osAggregator.insertResult(ipheader->saddr, detail);
    //osSamples.addToSample(ipheader->saddr, fingerprint.m_TCP_Options.tstamp, p->timestamp, fingerprint.m_IP_ID, fingerprint.m_TCP_SEQ);
}

string OSFPPlugin::parseTCPOptions(struct TCPOptions &options, const Packet* p, const uint32_t dataOffset){
    int optionsCnt = 0;
    bool lastFound = 0;
    uint8_t len = 0;
    uint8_t* len_ptr = &len;
    u_char op = 255;
    stringstream optionsStream;
    const u_char *option_ptr = p->transportHeader + TCP_HEADER_WITHOUT_OPTIONS;
    const u_char *const endOfOptions = p->transportHeader + dataOffset;

    while(!lastFound && option_ptr < endOfOptions)
    {
        op = (u_char) *option_ptr;
        switch(op)
        {
        case TCPOPT_NOP:
            options.nop_set += 1;
            optionsStream << "N";
            pkt_ignore_u8(&option_ptr);
            optionsCnt++;
            break;
        case TCPOPT_WINDOW:
            options.window_set = 1;
            if (option_ptr + 3 <= endOfOptions){
                pkt_ignore_u16(&option_ptr);
                pkt_get_u8(&option_ptr, &options.window_scale);
                optionsStream << "W";
                if (options.window_scale == 0){
                    optionsStream << "@0";
                }
            } else {
                // TCP Options corrupt, stopping this
                lastFound = 1;
                options.options_corrupt = 1;
            }
            optionsCnt++;
            break;
        case TCPOPT_MAXSEG:
            options.mss_set = 1;
            if (option_ptr + 4 <= endOfOptions){
                pkt_ignore_u16(&option_ptr);
                pkt_get_u16(&option_ptr, &options.mss);
                optionsStream << "M@" << options.mss;
            } else {
                // TCP Options corrupt, stopping this
                lastFound = 1;
                options.options_corrupt = 1;
            }
            optionsCnt++;
            break;
        case TCPOPT_TIMESTAMP:
            options.timestamp_set = 1;
            optionsStream << "T";
            if (option_ptr + 10 <= endOfOptions){
                pkt_ignore_u16(&option_ptr);
                pkt_get_u32(&option_ptr, &options.tstamp);
                pkt_get_u32(&option_ptr, &options.tsecr);
                if (options.tstamp == 0){
                    optionsStream << "@0";
                }
            } else {
                // TCP Options corrupt, stopping this
                lastFound = 1;
                options.options_corrupt = 1;
            }
            optionsCnt++;
            break;
        case TCPOPT_SACK_PERMITTED:
            options.sack_set = 1;
            optionsStream << "S";
            if (option_ptr + 2 <= endOfOptions){
                pkt_ignore_u16(&option_ptr);
            } else {
                // TCP Options corrupt, stopping this
                lastFound = 1;
                options.options_corrupt = 1;
            }
            optionsCnt++;
            break;
        case TCPOPT_EOL:
            options.eol_set = 1;
            pkt_ignore_u8(&option_ptr);
            optionsCnt++;
            lastFound = 1;
            break;
        default:
            options.unkown_option_set = 1;
            if (option_ptr + 2 <= endOfOptions){
                pkt_ignore_u8(&option_ptr);
                pkt_get_u8(&option_ptr, len_ptr);
                if (option_ptr + (len-2) <= endOfOptions){
                    option_ptr += len-2;
                } else {
                    // TCP Options corrupt, stopping this
                    lastFound = 1;
                    options.options_corrupt = 1;
                }
            } else {
                // TCP Options corrupt, stopping this
                lastFound = 1;
                options.options_corrupt = 1;
            }
            optionsCnt++;
            break;
        }
    }
    return optionsStream.str();
}

void OSFPPlugin::writeToFile(OSFingerprint* fingerprint){

    if (!filestream.is_open()){
        filestream.open(dumpFile.c_str(), ios_base::app);
    }
    filestream << fingerprint->toString().c_str();
}

void OSFPPlugin::initializeAggregator(uint32_t interval, std::string mode, std::string outputfile) {
    // the interval is mandatory for the OSResultAggregator
    if (interval > 0){

        osAggregator.setInterval(interval);

        // if the output mode is set to file, set mode and file
        if (mode == "file") {

            osAggregator.setOutputMode(OSResultAggregator::File);

            if (!outputfile.empty()) {
                osAggregator.setOuputFile(outputfile);
            } else {
                msg(MSG_ERROR, "OSResultAggregator output file is missing. Using console mode instead");

                osAggregator.setOutputMode(OSResultAggregator::Console);
            }

        } else {
            osAggregator.setOutputMode(OSResultAggregator::Console);
        }
    } else {
        msg(MSG_ERROR, "Unable to instantiate OSResultAggregator. Please provide a interval. Skipping aggregation!");
        return;
    }

    osAggregator.startExporterThread();
}

#endif
