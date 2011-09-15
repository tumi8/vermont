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

OSFPPlugin::OSFPPlugin(const u_int32_t maxPckts, std::string file){
    maxPackets = maxPckts;
    dumpFile = file;
    msg(MSG_INFO, "OSFPPlugin instantiated");
    msg(MSG_INFO, "  - dump file: %s", dumpFile.c_str());
    msg(MSG_INFO, "  - max connections: %i", maxPackets);
    myfile.open(dumpFile.c_str(), ios_base::out);
    myfile << "PCAP Timestamp : IP Source : TCP Source Port : IP Dest : TCP Dest Port : IP IHL : IP TOS : IP ID : "<<
              "IP Protocol : IP Dont Fragment : IP TTL : TCP Window Size : TCP MSS : TCP Window Scale : TCP SACK permitted Option : "<<
              "TCP NOP Option : TCP Timestamp set : TCP Timestamp : TCP Timestamp Secr : TCP Urgent Flag : "<<
              "TCP Push Flag : TCP Reset Flag : TCP FIN Flag : TCP Urgent Pointer : TCP EOL Option set : TCP Data Offset : " <<
              "TCP Sequence Number : TCP Acknowlegment Number : TCP Reserved 1 : TCP Reserved 2 : TCP Syn ACK None : LEN : Options Corrupt" << endl;
}

OSFPPlugin::~OSFPPlugin(){
    myfile.close();
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
    u_int16_t doffval;

    doff = p->transportHeader + 12;
    doffval = (u_int16_t)((*doff) >> 4);

    if (doffval > 5){
        options.mss = 1500 - (doffval * 4 + sizeof(p->netHeader));
        options.has_options = 1;

        /*parse option fields:*/
        parseTCPOptions(options, p, doffval*4);

    }
    writeToFile(options, p);
}

int OSFPPlugin::parseTCPOptions(struct TCPOptions &options, const Packet* p, const u_int32_t dataOffset){
    int optionsCnt = 0;
    bool lastFound = 0;
    u_int8_t len = 0;
    u_int8_t* len_ptr = &len;
    u_char op = 255;
    const u_char *option_ptr = p->transportHeader + TCP_HEADER_WITHOUT_OPTIONS;
    const u_char *const endOfOptions = p->transportHeader + dataOffset;

    while(!lastFound && option_ptr < endOfOptions)
    {
        op = (u_char) *option_ptr;
        switch(op)
        {
        case TCPOPT_NOP:
            options.nop_set += 1;
            pkt_ignore_u8(&option_ptr);
            optionsCnt++;
            break;
        case TCPOPT_WINDOW:
            options.window_set = 1;
            if (option_ptr + 3 <= endOfOptions){
                pkt_ignore_u16(&option_ptr);
                pkt_get_u8(&option_ptr, &options.window_scale);
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
            } else {
                // TCP Options corrupt, stopping this
                lastFound = 1;
                options.options_corrupt = 1;
            }
            optionsCnt++;
            break;
        case TCPOPT_TIMESTAMP:
            options.timestamp_set = 1;
            if (option_ptr + 10 <= endOfOptions){
                pkt_ignore_u16(&option_ptr);
                pkt_get_u32(&option_ptr, &options.tstamp);
                pkt_get_u32(&option_ptr, &options.tsecr);
            } else {
                // TCP Options corrupt, stopping this
                lastFound = 1;
                options.options_corrupt = 1;
            }
            optionsCnt++;
            break;
        case TCPOPT_SACK_PERMITTED:
            options.sack_set = 1;
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
    return optionsCnt;
}

void OSFPPlugin::writeToFile(struct TCPOptions &options, const Packet* p){
    bool test = false;
    const iphdr* ipheader;
    const tcphdr* tcpheader;

    if (!myfile.is_open()){
        myfile.open(dumpFile.c_str(), ios_base::app);
    }


    ipheader = (iphdr*) p->netHeader;
    tcpheader = (tcphdr*) p->transportHeader;

    /* PCAP Timestamp */
    myfile << p->timestamp.tv_sec << ".";
    myfile << p->timestamp.tv_usec << ":";
    /* IP Source */
    struct in_addr saddr;
    saddr.s_addr = ipheader->saddr;
    myfile << inet_ntoa(saddr) << ":";
    /* TCP Source Port */
    myfile << ntohs(tcpheader->source) << ":";
    /* IP Dest */
    struct in_addr daddr;
    daddr.s_addr = (in_addr_t) ipheader->daddr;
    myfile << inet_ntoa(daddr) << ":";
    /* TCP Dest Port */
    myfile << ntohs(tcpheader->dest) << ":";
    /* IP IHL */
    myfile << ipheader->ihl * 4 << ":";
    /* IP TOS */
    myfile << (u_int16_t) ipheader->tos << ":";
    /* IP ID */
    myfile << ntohs(ipheader->id) << ":";
    /* IP Protocol */
    myfile << (u_int16_t) ipheader->protocol << ":";
    /* IP Don't Fragment */
    myfile << (bool) ntohs(IP_DF) << ":";
    /* IP TTL */
    myfile << (u_int16_t) ipheader->ttl << ":";
    /* TCP Window Size */
    myfile << ntohs(tcpheader->window) << ":";
    /* TCP MSS */
    myfile << options.mss << ":";
    /* TCP Window Scale */
    myfile << (u_int16_t) options.window_scale << ":";
    /* TCP SACK permitted Option */
    myfile << (bool) options.sack_set << ":";
    /* TCP NOP Option */
    myfile << options.nop_set << ":";
    /* TCP Timestamp set */
    myfile << (bool) options.timestamp_set << ":";
    /* TCP Timestamp */
    myfile << options.tstamp << ":";
    /* TCP Timestamp Secr */
    myfile << options.tsecr << ":";
    /* TCP Urgent Flag */
    myfile << (bool) tcpheader->urg << ":";
    /* TCP Push Flag */
    myfile << (bool) tcpheader->psh << ":";
    /* TCP Reset Flag */
    myfile << (bool) tcpheader->rst << ":";
    /* TCP FIN Flag */
    myfile << (bool) tcpheader->fin << ":";
    /* TCP Urgent Pointer */
    myfile << (bool) tcpheader->urg_ptr << ":";
    /* TCP EOL Option set */
    myfile << (bool) options.eol_set << ":";
    /* TCP Data Offset */
    myfile << ntohs(tcpheader->doff) << ":";
    /* TCP Sequence Number */
    myfile << ntohl(tcpheader->seq) << ":";
    /* TCP Acknowlegment Number */
    myfile << ntohl(tcpheader->ack_seq) << ":";
    /* TCP Reserved 1 */
    myfile << ntohs(tcpheader->res1) << ":";
    /* TCP Reserved 2 */
    myfile << ntohs(tcpheader->res2) << ":";

    if((bool)tcpheader->syn == true && (bool)tcpheader->ack == true){
        test = 1;
        myfile << ("SA:");
    }
    if((bool)tcpheader->syn == true && (bool)tcpheader->ack == false){
        test = 1;
        myfile << ("S:");
    }
    if(test == false){
        myfile << ("A:");
    }
    /*TCP Packet Length*/
    myfile << p->data_length << ":";
    /* Options Corrupt */
    myfile << (bool) options.options_corrupt;
    myfile << endl;
}
#endif
