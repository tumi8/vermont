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
#ifndef OSFINGERPRINTDETECTION_H_
#define OSFINGERPRINTDETECTION_H_
#include "OSFingerprint.h"
#include <boost/unordered_map.hpp>
#include "common/Thread.h"

typedef boost::unordered_map <std::size_t, std::vector<OSFingerprint> > flowmap_t;
typedef boost::unordered_map <std::size_t, uint32_t> expirymap_t;

/**
  * TCP/IP Fingerprint Detection
  **/
class OSFingerprintDetection{

public:
    enum e_fp_type{
        SYN = 1,
        SYNACK,
        ACK
    };
    OSFingerprintDetection();
    ~OSFingerprintDetection();

    void detectSinglePacket(OSFingerprint *fp);
    void addFingerprintToFlow(OSFingerprint *fp);

private:
    flowmap_t flowmap;
    expirymap_t expirymap;
    std::size_t generateFlowKeyHash(OSFingerprint *fp);
    bool getFingerprint(std::size_t hash, e_fp_type type, OSFingerprint &fp);
    void detectPacketFlow(OSFingerprint fpSyn, OSFingerprint fpSynAck, OSFingerprint fpAck);
    Thread thread;
    uint32_t pollInterval; /**< polling interval in milliseconds */
    uint32_t flowexpiry;
    static void* threadWrapper(void* instance);
    void gcThread();
};
#endif
#endif


