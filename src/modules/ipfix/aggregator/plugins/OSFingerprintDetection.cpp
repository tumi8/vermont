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
#include "OSFingerprintDetection.h"
#include <boost/functional/hash.hpp>
#include "common/Time.h"
#include <time.h>
#include <errno.h>

OSFingerprintDetection::OSFingerprintDetection()
    :thread(OSFingerprintDetection::threadWrapper),
      pollInterval(1000),
      flowexpiry(120)
{
    thread.run(this);
}

OSFingerprintDetection::~OSFingerprintDetection() {
    thread.exitFlag = true;
    thread.join();
}

void OSFingerprintDetection::gcThread() {
    struct timeval inttimer;
    gettimeofday(&inttimer, 0);
    flowmap_t::iterator itBegin;
    flowmap_t::iterator itEnd;
    flowmap_t::iterator itTemp;
    while (!thread.exitFlag) {
        addToCurTime(&inttimer, pollInterval);

        struct timeval curtime;
        gettimeofday(&curtime, 0);

        struct timeval difftime;
        if (timeval_subtract(&difftime, &inttimer, &curtime)!=1) {
            // restart nanosleep with the remaining sleep time
            // if we got interrupted by a signal
            struct timespec ts;
            TIMEVAL_TO_TIMESPEC(&difftime, &ts);
            while (nanosleep(&ts, &ts) == -1 && errno == EINTR);
        }

        gettimeofday(&curtime, 0);

        itBegin = flowmap.begin();
        itEnd = flowmap.end();
        // while loop used to be able to delete while iterating
        while (itBegin != itEnd)
        {
            if (expirymap[itBegin->first] < curtime.tv_sec - flowexpiry)
            {
                //reset expiry
                expirymap.erase(itBegin->first);
                itTemp = itBegin;
                ++itBegin;
                //reset result
                flowmap.erase(itTemp);
            } else {
                ++itBegin;
            }
        }
        struct timeval endtime;
        gettimeofday(&endtime, 0);
        timeval_subtract(&difftime, &endtime, &curtime);
    }
}

void* OSFingerprintDetection::threadWrapper(void* instance){
    OSFingerprintDetection* osfd = reinterpret_cast<OSFingerprintDetection*>(instance);
    osfd->gcThread();
    return 0;
}

std::size_t OSFingerprintDetection::generateFlowKeyHash(OSFingerprint *fp) {
    in_addr_t ip1, ip2;
    u_int16_t port1,port2;

    if(fp->m_IP_Source.s_addr <= fp->m_IP_Dest.s_addr){
        ip1 = fp->m_IP_Source.s_addr;
        port1 = fp->m_TCP_SourcePort;
        ip2 = fp->m_IP_Dest.s_addr;
        port2 = fp->m_TCP_DestPort;
    }
    else{
        ip2 = fp->m_IP_Source.s_addr;
        port2 = fp->m_TCP_SourcePort;
        ip1 = fp->m_IP_Dest.s_addr;
        port1 = fp->m_TCP_DestPort;
    }

    std::size_t seed = 0;
    boost::hash_combine(seed, ip1);
    boost::hash_combine(seed, ip2);
    boost::hash_combine(seed, port1);
    boost::hash_combine(seed, port2);
    boost::hash_combine(seed, fp->m_IP_Protocol);
    return seed;
}

/**
  * Search fingerprint in Flow (SYN or SYNACK fingerprint)
  **/
bool OSFingerprintDetection::getFingerprint(std::size_t hash, e_fp_type type, OSFingerprint &fp) {
    if (flowmap.find(hash) != flowmap.end()) {
        if (type == SYN && flowmap[hash][0].m_TCP_SYN) {
            fp = flowmap[hash][0];
            return true;
        }
        if (type == SYNACK && flowmap[hash][1].m_TCP_SYN && flowmap[hash][1].m_TCP_ACK) {
            fp = flowmap[hash][1];
            return true;
        }
        if (type == ACK && !flowmap[hash][2].m_TCP_SYN && flowmap[hash][2].m_TCP_ACK) {
            fp = flowmap[hash][2];
            return true;
        }
    }
    return false;
}

void OSFingerprintDetection::addFingerprintToFlow(OSFingerprint *fp) {
    std::size_t hash;
    struct timeval curtime;
    gettimeofday(&curtime, 0);

    hash = generateFlowKeyHash(fp);

    //if hash is found, a SYN packet was already seen
    if (flowmap.find(hash) != flowmap.end()) {
        OSFingerprint fpSyn, fpSynAck, fpAck;
        bool syn_set, synack_set, ack_set;
        syn_set = getFingerprint(hash, SYN, fpSyn);
        synack_set = getFingerprint(hash, SYNACK, fpSynAck);
        ack_set = false;

        // add SYNACK fingerprint if it belongs to the flow
        if (fp->m_TCP_SYN && fp->m_TCP_ACK && syn_set && (fpSyn.m_TCP_SEQ + 1) == fp->m_TCP_ACK_SEQ && !synack_set) {
            flowmap[hash][1] = *fp;
            expirymap[hash] = curtime.tv_sec;
        }

        // add ACK fingerprint if it belongs to the flow
        if (!fp->m_TCP_SYN && fp->m_TCP_ACK && synack_set && !ack_set && fpSynAck.m_TCP_ACK_SEQ == fp->m_TCP_SEQ) {
            flowmap[hash][2] = *fp;
            expirymap[hash] = curtime.tv_sec;
            ack_set = getFingerprint(hash, ACK, fpAck);
        }

        if (syn_set && synack_set && ack_set && fpSyn.m_Pcap_Timestamp.tv_sec + flowexpiry > curtime.tv_sec) {
            detectPacketFlow(fpSyn, fpSynAck, fpAck);
            // expire flow, will be delete by gcThread
            expirymap[hash] -= flowexpiry;
        }

    } else {
        // only add SYN packets
        if (fp->m_TCP_SYN && !fp->m_TCP_ACK) {
            flowmap[hash] = vector<OSFingerprint> (3);
            flowmap[hash][0]= *fp;
            expirymap[hash] = (uint32_t)fp->m_Pcap_Timestamp.tv_sec;
        }
    }
}

/**
  * try to detect the Operating System using Weka decision trees for S-SA-A flows
  **/
void OSFingerprintDetection::detectPacketFlow(OSFingerprint fpSyn, OSFingerprint fpSynAck, OSFingerprint fpAck) {
    //make sure all three packets have been received
//    if (!fpSyn || !fpSynAck || !fpAck) {
//        printf("Some fingerprint is missing");
//    }
    // Put OS detection code here
    printf("SYN: %s\n", fpSyn.toString().c_str());
    printf("SYNACK: %s\n", fpSynAck.toString().c_str());
    printf("ACK: %s\n", fpAck.toString().c_str());
    // End of detection code
}

/**
  * try to detect the Operating System using Weka decision trees
  **/
void OSFingerprintDetection::detectSinglePacket(OSFingerprint* fp) {
    // Put OS detection code here

    // End of detection code
}

#endif
