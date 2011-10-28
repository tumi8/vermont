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
#include "OSFingerprint.h"
#include "OSDetail.h"
#include <sstream>
#include <string>
#include <arpa/inet.h>
using namespace std;

OSFingerprint::OSFingerprint()
    : m_IP_TOS(0), m_IP_ID(0), m_IP_DF(0), m_IP_TTL(0), m_TCP_Window_Size(0), m_TCP_URG(0),
      m_TCP_PSH(0), m_TCP_RST(0), m_TCP_FIN(0), m_TCP_URG_PTR(0), m_TCP_DOFF(0), m_TCP_SEQ(0),
      m_TCP_ACK_SEQ(0), m_TCP_RES1(0), m_TCP_RES2(0), m_TCP_SYN(0), m_TCP_ACK(0), m_Data_Length(0),
      m_OS_Type(""), m_OS_Version(""), ordered_options("")
{
    memset(&m_TCP_Options, 0, sizeof(struct TCPOptions));
    m_TCP_Options.mss = 1460;
}


/*
 * create string representation of a specific fingerprint
 */
string OSFingerprint::toString() const{
    ostringstream strm;
    bool test = false;
    string san_flag = "A";

    /* PCAP Timestamp */
    strm << m_Pcap_Timestamp.tv_sec << ".";
    strm << m_Pcap_Timestamp.tv_usec << ":";
    /* IP Source */
    strm << inet_ntoa(m_IP_Source) << ":";
    /* TCP Source Port */
    strm << m_TCP_SourcePort << ":";
    /* IP Dest */
    strm << inet_ntoa(m_IP_Dest) << ":";
    /* TCP Dest Port */
    strm << m_TCP_DestPort << ":";
    /* IP IHL */
    strm << m_IP_IHL * 4 << ":";
    /* IP TOS */
    strm << m_IP_TOS << ":";
    /* IP ID */
    strm << m_IP_ID << ":";
    /* IP Protocol */
    strm << m_IP_Protocol << ":";
    /* IP Don't Fragment */
    strm << m_IP_DF << ":";
    /* IP TTL */
    strm << m_IP_TTL << ":";
    /* TCP Window Size */
    strm << m_TCP_Window_Size<< ":";
    /* TCP MSS */
    strm << m_TCP_Options.mss << ":";
    /* TCP Window Scale */
    strm << (uint16_t) m_TCP_Options.window_scale << ":";
    /* TCP SACK permitted Option */
    strm << (bool) m_TCP_Options.sack_set << ":";
    /* TCP NOP Option */
    strm << m_TCP_Options.nop_set << ":";
    /* TCP Timestamp set */
    strm << (bool) m_TCP_Options.timestamp_set << ":";
    /* TCP Timestamp */
    strm << m_TCP_Options.tstamp << ":";
    /* TCP Timestamp Secr */
    strm << m_TCP_Options.tsecr << ":";
    /* TCP Urgent Flag */
    strm << m_TCP_URG << ":";
    /* TCP Push Flag */
    strm << m_TCP_PSH << ":";
    /* TCP Reset Flag */
    strm << m_TCP_RST << ":";
    /* TCP FIN Flag */
    strm << m_TCP_FIN << ":";
    /* TCP Urgent Pointer */
    strm << m_TCP_URG_PTR << ":";
    /* TCP EOL Option set */
    strm << (bool) m_TCP_Options.eol_set << ":";
    /* TCP Data Offset */
    strm << m_TCP_DOFF << ":";
    /* TCP Sequence Number */
    strm << m_TCP_SEQ << ":";
    /* TCP Acknowlegment Number */
    strm << m_TCP_ACK_SEQ << ":";
    /* TCP Reserved 1 */
    strm << m_TCP_RES1 << ":";
    /* TCP Reserved 2 */
    strm << m_TCP_RES2 << ":";

    if(m_TCP_SYN && m_TCP_ACK){
        test = true;
        san_flag = "SA";
    }
    if(m_TCP_SYN && !m_TCP_ACK){
        test = true;
        san_flag = "S";
    }
    if(!test){
        san_flag = "A";
    }
    strm << san_flag << ":";

    /*TCP Packet Length*/
    strm << m_Data_Length << ":";
    /* Options Corrupt */
    strm << (bool) m_TCP_Options.options_corrupt << ":";
    if ( m_TCP_Options.has_options ){
        strm << ordered_options;
    } else {
        strm << "?";
    }

    return strm.str();
}

uint16_t OSFingerprint::guessInitialTTL(uint8_t ttl){
    if (ttl <= 32)
        return 32;
    else if (ttl <= 64)
        return 64;
    else if (ttl <= 128)
        return 128;
    else
        return 255;
}

/*
 * try to detect the Operating System using Weka decision trees
 */
void OSFingerprint::detectOS(){
    if (m_TCP_SYN) {
        if (m_TCP_Window_Size <= 65532) {
            if (m_IP_TTL > 64) {
                m_OS_Type = "Windows";
            } else {
                m_OS_Type = "Linux";
            }
        } else { // Mac seems to always set 65535 as WS
            m_OS_Type = "Mac";
        }
    }

    printf("%s: %s %u\n", m_OS_Type.c_str(), inet_ntoa(m_IP_Source), m_TCP_Window_Size);

    // put OS detection code here
}

#endif
