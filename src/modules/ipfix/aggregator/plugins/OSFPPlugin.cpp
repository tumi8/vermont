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
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <iostream>
#include <fstream>
using namespace std;

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

void OSFPPlugin::flowDeleted(const HashtableBucket* bucket){
    msg(MSG_INFO, "flow deleted");
}

void OSFPPlugin::newFlowReceived(const HashtableBucket* bucket){
    msg(MSG_INFO, "new flow");
}

void OSFPPlugin::newPacketReceived(const Packet* p){
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
        optionsCnt = parseTCPOptions(options, p);

    }

    /*u_int16_t window = ntohs(tcpheader->window);
    if (mss && window && !(window % mss)) printf("S%d", window/mss); else
        if (window && !(window % 1460)) printf("S%d", window/1460); else
            if (mss && window && !(window % (mss+40))) printf("T%d", window/(mss+40)); else
                if (window && !(window % 1500)) printf("T%d", window/1500); else
                    if (window == 12345) printf("*(12345)"); else printf("%d", window);
    */
    writeToFile(options, p);
}

int OSFPPlugin::parseTCPOptions(struct TCPOptions &options, const Packet* p){
    int optionsCnt = 0;
    short lastFound = 0;
    u_char len;
    u_char op;
    const u_char *option_ptr = p->transportHeader + sizeof(struct tcphdr);

    while(!lastFound && optionsCnt<100)
    {
        op = (u_char) *option_ptr;
        switch(op)
        {
        case TCPOPT_NOP:
            options.nop_set += 1;
            option_ptr++;
            optionsCnt++;
            break;
        case TCPOPT_WINDOW:
            options.window_set = 1;
            if (optionsCnt>1){
                option_ptr++;
                len = *option_ptr;
                option_ptr++;
                memcpy(&options.window_scale, option_ptr, 1);
                //options.window_scale = ntohs(options.window_scale);
                option_ptr += len-2;;
            }
            optionsCnt++;
            break;
        case TCPOPT_MAXSEG:
            options.mss_set = 1;
            if (optionsCnt>1){
                option_ptr++;
                len = *option_ptr;
                option_ptr++;
                memcpy(&options.mss, option_ptr, 2);
                options.mss = ntohs(options.mss);
                //options.mss -= tcphdr_size;
                option_ptr += len-2;
            }
            optionsCnt++;
            break;
        case TCPOPT_TIMESTAMP:
            options.timestamp_set = 1;
            if (optionsCnt>1){
                option_ptr++;
                len = *option_ptr;
                option_ptr++;
                memcpy(&options.tstamp, option_ptr, 4);
                options.tstamp = ntohl(options.tstamp);
                memcpy(&options.tsecr, option_ptr+4, 4);
                options.tsecr = ntohl(options.tsecr);
                option_ptr += len -2;;
            }
            optionsCnt++;
            break;
        case TCPOPT_SACK_PERMITTED:
            options.sack_set = 1;
            if (optionsCnt>1){
                option_ptr++;
                len = *option_ptr;
                option_ptr++;
                option_ptr += len -2;
            }
            optionsCnt++;
            break;
            //case TCPOLEN_TIMESTAMP: printf("TSLEN, value: %d", ntohs(options[++offset])); optionsCnt++; break;
        case TCPOPT_EOL:
            options.eol_set = 1;
            optionsCnt++;
            lastFound = 1;
            break;
        default:
            options.unkown_option_set = 1;
            if (optionsCnt>1){
                option_ptr++;
                len = *option_ptr;
                option_ptr++;
                option_ptr += len -2;
            }
            optionsCnt++;
            break;
        }
    }
    return optionsCnt;
}

void OSFPPlugin::writeToFile(struct TCPOptions &options, const Packet* p){
    bool test = false;
    const struct ip* ipheader;
    const struct tcphdr* tcpheader;
    ofstream myfile;

    if (writeHeaderFlag == 1){
        writeHeaderFlag = 0;
        /* print file header */
        myfile.open(dumpFile.c_str(), ios_base::out);
        myfile << "PCAP Timestamp : IP Source : TCP Source Port : IP Dest : TCP Dest Port : IP IHL : IP TOS : IP ID : "<<
                  "IP Protocol : IP Dont Fragment : IP TTL : TCP Window Size : TCP MSS : TCP Window Scale : TCP SACK permitted Option : "<<
                  "TCP NOP Option : TCP Timestamp set : TCP Timestamp : TCP Timestamp Secr : TCP Urgent Flag : "<<
                  "TCP Push Flag : TCP Reset Flag : TCP FIN Flag : TCP Urgent Pointer : TCP EOL Option set : TCP Data Offset : " <<
                  "TCP Reserved 1 : TCP Reserved 2 : TCP Syn ACK None : LEN\n";
        myfile.close();
    }


    ipheader = (struct ip*) p->netHeader;
    tcpheader = (struct tcphdr*) p->transportHeader;

    myfile.open(dumpFile.c_str(), ios_base::app);

    /* PCAP Timestamp */
    myfile << to_string<long>(p->timestamp.tv_sec, std::dec) << ".";
    myfile << to_string<long>(p->timestamp.tv_usec, std::dec) << ":";
    /* IP Source */
    myfile << inet_ntoa(ipheader->ip_src) << ":";
    /* TCP Source Port */
    myfile << to_string<u_int16_t>(ntohs(tcpheader->source), std::dec) << ":";
    /* IP Dest */
    myfile << inet_ntoa(ipheader->ip_dst) << ":";
    /* TCP Dest Port */
    myfile << to_string<u_int16_t>(ntohs(tcpheader->dest), std::dec) << ":";
    /* IP IHL */
    myfile << to_string<u_int16_t>(ipheader->ip_hl * 4, std::dec) << ":";
    /* IP TOS */
    myfile << to_string<u_int16_t>(ipheader->ip_tos, std::dec) << ":";
    /* IP ID */
    myfile << to_string<u_short>(ntohs(ipheader->ip_id), std::dec) << ":";
    /* IP Protocol */
    myfile << to_string<u_int16_t>(ipheader->ip_p, std::dec) << ":";
    /* IP Don't Fragment */
    myfile << to_string<bool>((bool) ntohs(IP_DF), std::dec) << ":";
    /* IP TTL */
    myfile << to_string<u_int16_t>(ipheader->ip_ttl, std::dec) << ":";
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
    myfile << to_string<u_int32_t>(p->data_length*4, std::dec) << "\n";
    //myfile << ("UnkownSystem\n");
    myfile.close();
}
