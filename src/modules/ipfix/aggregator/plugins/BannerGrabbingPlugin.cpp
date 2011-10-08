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
#include "BannerGrabbingPlugin.h"
#include "common/msg.h"
#include "modules/ipfix/aggregator/PacketHashtable.h"
#include "modules/ipfix/aggregator/plugins/PacketFunctions.h"
#include <arpa/inet.h>
#include <features.h>
#include <boost/regex.hpp>

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

BannerGrabbingPlugin::BannerGrabbingPlugin(){
    BannerGrabbingPlugin(0, (char*) "dump_http.csv", "");
}


BannerGrabbingPlugin::BannerGrabbingPlugin(const uint32_t maxPckts, std::string file, std::string bannerfile):performOSGuessing(false){
    maxPackets = maxPckts;
    dumpFile = file;
    msg(MSG_INFO, "BannerGrabbingPlugin instantiated");
    msg(MSG_INFO, "  - dump file: %s", dumpFile.c_str());

    myfile.open(dumpFile.c_str(), ios_base::out);
    myfile << "PCAP Timestamp : IP : Banner\n";
    if (!bannerfile.empty()){
        banners = BannerOSMapping::getBanners(bannerfile);
        performOSGuessing = true;
    }
}

BannerGrabbingPlugin::~BannerGrabbingPlugin(){
    myfile.close();
}


void BannerGrabbingPlugin::flowDeleted(const HashtableBucket* bucket){
    map.erase(bucket->hash);
}


void BannerGrabbingPlugin::newFlowReceived(const HashtableBucket* bucket){
    map[bucket->hash] = 0;
}


void BannerGrabbingPlugin::newPacketReceived(const Packet* p, uint32_t hash){

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

    processPacket(p);
}

/*
 * process the Packet, ie. search for a banner in the payload of the packet
 */
void BannerGrabbingPlugin::processPacket(const Packet *p){
    const char* payload;
    payload = (const char*) p->payload;

    if (payload == NULL){
        return;
    }

    //HTTP Header Regex
    static const boost::regex ex("User-Agent\\s*: (.*)");
    //SSH Regex
    static const boost::regex exSSH("SSH.*");

    uint32_t payload_len = PCAP_MAX_CAPTURE_LENGTH - p->payloadOffset;
    std::string inputStr(payload, payload_len);
    std::string::const_iterator start, end;
    start = inputStr.begin();
    end = inputStr.end();



    try{
        boost::match_results<std::string::const_iterator> what;
        boost::match_flag_type flags = boost::match_not_dot_newline;

        //if http regex found --> save result
        if(boost::regex_search(start, end, what, ex, flags))
        {
            std::string result_stringHTTP(what[1].str());
            saveResult(p, &result_stringHTTP);
            if (performOSGuessing) guessOS(p, &result_stringHTTP, HTTP);
        }

        //if ssh regex found --> save result
        if(boost::regex_search(start, end, what, exSSH, flags))
        {
            std::string result_stringSSH(what[0].str());
            saveResult(p, &result_stringSSH);
            if (performOSGuessing) guessOS(p, &result_stringSSH, SSH);
        }
    }
    catch(std::runtime_error& e){
        printf("%s", e.what());
    }
}

/*
 * save results into a csv file
 */
void BannerGrabbingPlugin::saveResult(const Packet* p, std::string* result_ptr){
    const iphdr* ipheader;

    std::string result = *result_ptr;

    if (!myfile.is_open()){
        myfile.open(dumpFile.c_str(), ios_base::app);
    }

    if (result != ""){
        ipheader = (iphdr*) p->netHeader;


        /* PCAP Timestamp */
        myfile << p->timestamp.tv_sec << ".";
        myfile << p->timestamp.tv_usec << ":";
        /* IP Source */
        struct in_addr saddr;
        saddr.s_addr = ipheader->saddr;
        myfile << inet_ntoa(saddr) << ":";
        if (p->ipProtocolType == Packet::TCP){
            const tcphdr* tcpheader;
            tcpheader = (tcphdr*) p->transportHeader;
            myfile << ntohs(tcpheader->source) << ":";
        } else {
            myfile << "unknown:";
        }

        /*Banner*/
        myfile << result;
        myfile << endl;
    }
}

void BannerGrabbingPlugin::guessOS(const Packet* p, std::string* grabbedString, e_bannerType type){
    std::list<BannerOSMapping>::iterator it;
    const iphdr* ipheader;
    ipheader = (iphdr*) p->netHeader;
    struct in_addr saddr;
    saddr.s_addr = ipheader->saddr;

    std::string searchString = *grabbedString;

    switch (type) {
    case HTTP:
    case SSH:
        for ( it=banners.begin() ; it != banners.end(); it++ ){
            BannerOSMapping banner = *it;

            if ( banner.osVersionMatches(searchString) ){
                osAggregator.insertResult(ipheader->saddr, OSDetail(banner.osType, banner.osVersion, banner.findArchitecture(searchString), BANNER));
                return;
            }
        }
        for ( it=banners.begin() ; it != banners.end(); it++ ){
            BannerOSMapping banner = *it;

            if ( banner.osTypeMatches(searchString) ){
                osAggregator.insertResult(ipheader->saddr, OSDetail(banner.osType, "", banner.findArchitecture(searchString), BANNER));
                return;
            }
        }
        break;
    default:
        break;
    }
}

void BannerGrabbingPlugin::processMsg(string message){
    printf(message.c_str());
}

#endif
