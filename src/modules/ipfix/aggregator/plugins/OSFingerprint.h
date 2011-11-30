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
#ifndef OSFINGERPRINT_H_
#define OSFINGERPRINT_H_
#include "modules/packet/Packet.h"
#include <boost/shared_ptr.hpp>
// TCP Options structure
struct TCPOptions{
    bool has_options;
    bool sack_set;
    bool timestamp_set;
    bool mss_set;
    bool window_set;
    bool eol_set;
    bool unkown_option_set;
    bool options_corrupt;
    uint32_t nop_set;
    uint32_t tstamp;
    uint32_t tsecr;
    uint8_t window_scale;
    uint16_t mss;
};

/**
  * Represents a TCP/IP Fingerprint
  **/
class OSFingerprint{

public:
    OSFingerprint();
    string toString() const;
    void detectOS();
    uint16_t guessInitialTTL(uint8_t ttl);

    struct timeval m_Pcap_Timestamp;
    TCPOptions m_TCP_Options;
    struct in_addr m_IP_Source;
    uint16_t m_TCP_SourcePort;
    struct in_addr m_IP_Dest;
    uint16_t m_TCP_DestPort;
    uint16_t m_IP_IHL;
    uint16_t m_IP_TOS;
    uint16_t m_IP_ID;
    uint16_t m_IP_Protocol;
    bool m_IP_DF;
    uint16_t m_IP_TTL;
    uint16_t m_TCP_Window_Size;
    bool m_TCP_URG;
    bool m_TCP_PSH;
    bool m_TCP_RST;
    bool m_TCP_FIN;
    bool m_TCP_URG_PTR;
    uint16_t m_TCP_DOFF;
    uint32_t m_TCP_SEQ;
    uint32_t m_TCP_ACK_SEQ;
    uint16_t m_TCP_RES1;
    uint16_t m_TCP_RES2;
    bool m_TCP_SYN;
    bool m_TCP_ACK;
    unsigned int m_Data_Length;
    string m_OS_Type;
    string m_OS_Version;
    string ordered_options;
public:
    typedef boost::shared_ptr<OSFingerprint> Ptr;
};

#endif
#endif

