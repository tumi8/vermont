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

#include "OSFPPlugin.h"
#include "common/msg.h"
#include <arpa/inet.h>
#include <iostream>
#include <fstream>
#include <features.h>
#include "modules/ipfix/aggregator/PacketHashtable.h"


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
    OSFPPlugin(0,(char*) "dump.csv");
}

OSFPPlugin::OSFPPlugin(const u_int32_t maxConns, std::string file){
    writeHeaderFlag = 1;
    maxConnections = maxConns;
    dumpFile = file;
    msg(MSG_INFO, "OSFPPlugin instantiated");
    msg(MSG_INFO, "  - dump file: %s", dumpFile.c_str());
    msg(MSG_INFO, "  - max connections: %i", maxConnections);
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
    /* is connection tracking needed?
       if yes, check if not more than maxConnections */
    if (maxConnections > 0){
        if (map.find(hash) != map.end()){
            ++map[hash];
        }
        else{
            map[hash] = 1;
        }
        if (map[hash] > maxConnections){
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
    int optionsCnt = 0;
    unsigned char *doff;
    u_int16_t doffval;

    doff = p->transportHeader + 12;
    doffval = (u_int16_t)((*doff) >> 4);

    if (doffval > 5){
        options.mss = 1500 - (doffval * 4 + sizeof(p->netHeader));
        options.has_options = 1;

        /*parse option fields:*/
        optionsCnt = parseTCPOptions(options, p, doffval*4);

    }
    writeToFile(options, p);
}

int OSFPPlugin::parseTCPOptions(struct TCPOptions &options, const Packet* p, const u_int32_t dataOffset){
    int optionsCnt = 0;
    short lastFound = 0;
    u_int8_t len = 0;
    u_int8_t* len_ptr = &len;
    u_char op = 255;
    const u_char *option_ptr = p->transportHeader + TCP_HEADER_WITHOUT_OPTIONS;
    const u_char *endOfOptions = p->transportHeader + dataOffset;

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
//        case TCPOPT_SACK:
//            break;
        case TCPOPT_EOL:
            options.eol_set = 1;
            pkt_ignore_u8(&option_ptr);
            optionsCnt++;
            lastFound = 1;
            break;
        default:
            options.unkown_option_set = 1;
            if (option_ptr + 1 <= endOfOptions){
                pkt_ignore_u8(&option_ptr);
                pkt_get_u8(&option_ptr, len_ptr);
                if (option_ptr + (len-2) <= endOfOptions){
                    option_ptr += len-2;
                }
                else{
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
    ofstream myfile;

    if (writeHeaderFlag == 1){
        writeHeaderFlag = 0;
        /* print file header */
        myfile.open(dumpFile.c_str(), ios_base::out);
        myfile << "PCAP Timestamp : IP Source : TCP Source Port : IP Dest : TCP Dest Port : IP IHL : IP TOS : IP ID : "<<
                  "IP Protocol : IP Dont Fragment : IP TTL : TCP Window Size : TCP MSS : TCP Window Scale : TCP SACK permitted Option : "<<
                  "TCP NOP Option : TCP Timestamp set : TCP Timestamp : TCP Timestamp Secr : TCP Urgent Flag : "<<
                  "TCP Push Flag : TCP Reset Flag : TCP FIN Flag : TCP Urgent Pointer : TCP EOL Option set : TCP Data Offset : " <<
                  "TCP Sequence Number : TCP Acknowlegment Number : TCP Reserved 1 : TCP Reserved 2 : TCP Syn ACK None : LEN : Options Corrupt\n";
        myfile.close();
    }


    ipheader = (iphdr*) p->netHeader;
    tcpheader = (tcphdr*) p->transportHeader;

    myfile.open(dumpFile.c_str(), ios_base::app);

    /* PCAP Timestamp */
    myfile << to_string<long>(p->timestamp.tv_sec, std::dec) << ".";
    myfile << to_string<long>(p->timestamp.tv_usec, std::dec) << ":";
    /* IP Source */
    struct in_addr saddr;
    saddr.s_addr = ipheader->saddr;
    myfile << inet_ntoa(saddr) << ":";
    /* TCP Source Port */
    myfile << to_string<u_int16_t>(ntohs(tcpheader->source), std::dec) << ":";
    /* IP Dest */
    struct in_addr daddr;
    daddr.s_addr = (in_addr_t) ipheader->daddr;
    myfile << inet_ntoa(daddr) << ":";
    /* TCP Dest Port */
    myfile << to_string<u_int16_t>(ntohs(tcpheader->dest), std::dec) << ":";
    /* IP IHL */
    myfile << to_string<u_int16_t>(ipheader->ihl * 4, std::dec) << ":";
    /* IP TOS */
    myfile << to_string<u_int16_t>(ipheader->tos, std::dec) << ":";
    /* IP ID */
    myfile << to_string<u_short>(ntohs(ipheader->id), std::dec) << ":";
    /* IP Protocol */
    myfile << to_string<u_int16_t>(ipheader->protocol, std::dec) << ":";
    /* IP Don't Fragment */
    myfile << to_string<bool>((bool) ntohs(IP_DF), std::dec) << ":";
    /* IP TTL */
    myfile << to_string<u_int16_t>(ipheader->ttl, std::dec) << ":";
    /* TCP Window Size */
    myfile << to_string<u_int16_t>(ntohs(tcpheader->window), std::dec) << ":";
    /* TCP MSS */
    myfile << to_string<u_int16_t>(options.mss, std::dec) << ":";
    /* TCP Window Scale */
    myfile << to_string<u_int16_t>(options.window_scale, std::dec) << ":";
    /* TCP SACK permitted Option */
    myfile << to_string<bool>(options.sack_set, std::dec) << ":";
    /* TCP NOP Option */
    myfile << to_string<u_int32_t>(options.nop_set, std::dec) << ":";
    /* TCP Timestamp set */
    myfile << to_string<bool>(options.timestamp_set, std::dec) << ":";
    /* TCP Timestamp */
    myfile << to_string<u_int32_t>(options.tstamp, std::dec) << ":";
    /* TCP Timestamp Secr */
    myfile << to_string<u_int32_t>(options.tsecr, std::dec) << ":";
    /* TCP Urgent Flag */
    myfile << to_string<bool>(tcpheader->urg, std::dec) << ":";
    /* TCP Push Flag */
    myfile << to_string<bool>(tcpheader->psh, std::dec) << ":";
    /* TCP Reset Flag */
    myfile << to_string<bool>(tcpheader->rst, std::dec) << ":";
    /* TCP FIN Flag */
    myfile << to_string<bool>(tcpheader->fin, std::dec) << ":";
    /* TCP Urgent Pointer */
    myfile << to_string<u_int16_t>(tcpheader->urg_ptr, std::dec) << ":";
    /* TCP EOL Option set */
    myfile << to_string<bool>(options.eol_set, std::dec) << ":";
    /* TCP Data Offset */
    myfile << to_string<u_int16_t>(ntohs(tcpheader->doff), std::dec) << ":";
    /* TCP Sequence Number */
    myfile << to_string<u_int32_t>(ntohl(tcpheader->seq), std::dec) << ":";
    /* TCP Acknowlegment Number */
    myfile << to_string<u_int32_t>(ntohl(tcpheader->ack_seq), std::dec) << ":";
    /* TCP Reserved 1 */
    myfile << to_string<u_int16_t>(ntohs(tcpheader->res1), std::dec) << ":";
    /* TCP Reserved 2 */
    myfile << to_string<u_int16_t>(ntohs(tcpheader->res2), std::dec) << ":";

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
    myfile << to_string<u_int32_t>(p->data_length, std::dec) << ":";
    /* Options Corrupt */
    myfile << to_string<bool>((bool) options.options_corrupt, std::dec) << "\n";
    //myfile << ("UnkownSystem\n");
    myfile.close();
}
