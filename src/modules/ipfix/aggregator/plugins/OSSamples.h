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
#ifndef OSSAMPLES_H_
#define OSSAMPLES_H_
#include <boost/unordered_map.hpp>
#include <vector>
#include <set>
#include "common/Time.h"
#include <time.h>

/* Based on NMAP utils.h */
/* Arithmatic difference modulo 2^32 */
#ifndef MOD_DIFF
#define MOD_DIFF(a,b) ((uint32_t) (std::min((uint32_t)(a) - (uint32_t ) (b), (uint32_t )(b) - (uint32_t) (a))))
#endif
#ifndef TIMEVAL_SUBTRACT
#define TIMEVAL_SUBTRACT(a,b) (((a).tv_sec - (b).tv_sec) * 1000000 + (a).tv_usec - (b).tv_usec)
#endif

/* Taken from NMAP osscan2.cc */
/* TCP Timestamp Sequence */
#define TS_SEQ_UNKNOWN 0
#define TS_SEQ_ZERO 1 /* At least one of the timestamps we received back was 0 */
#define TS_SEQ_2HZ 2
#define TS_SEQ_100HZ 3
#define TS_SEQ_1000HZ 5
#define TS_SEQ_OTHER_NUM 6
#define TS_SEQ_UNSUPPORTED 7 /* System didn't send back a timestamp */

#define IPID_SEQ_UNKNOWN 0
#define IPID_SEQ_INCR 1  /* simple increment by one each time */
#define IPID_SEQ_BROKEN_INCR 2 /* Stupid MS -- forgot htons() so it
                                  counts by 256 on little-endian platforms */
#define IPID_SEQ_RPI 3 /* Goes up each time but by a "random" positive
                          increment */
#define IPID_SEQ_RD 4 /* Appears to select IPID using a "random" distributions (meaning it can go up or down) */
#define IPID_SEQ_CONSTANT 5 /* Contains 1 or more sequential duplicates */
#define IPID_SEQ_ZERO 6 /* Every packet that comes back has an IP.ID of 0 (eg Linux 2.4 does this) */

/* Maximum number of samples stored in the maps */
#define NUM_MAX_SAMPLES 3

struct seq_info{
    uint32_t ts_seq_class;
    uint32_t ipid_class;
    int index;
    int timestamps[NUM_MAX_SAMPLES];
    timeval capturetimes[NUM_MAX_SAMPLES];
    int ipids[NUM_MAX_SAMPLES];
    uint32_t seq_numbers[NUM_MAX_SAMPLES];
    std::set<uint32_t> ips;
};

struct rr_info{
    uint32_t syn_ack_seq_number;
    uint32_t syn_seq_number;
};

typedef boost::unordered_map< uint32_t, struct seq_info > ts_map;
typedef boost::unordered_map< std::size_t, struct rr_info > rr_map;

class OSFingerprint;


class OSSamples{

public:
    void addToSample(uint32_t hash, OSFingerprint* fingerprint, timeval capturetime);
    void predictSequences(uint32_t ip, uint32_t hash);
    void forceOSSamplePrediction(uint32_t hash);

private:
    uint32_t gcd_n_uint(int nvals, unsigned int *val);
    int get_ipid_sequence(int numSamples, int *ipids, int islocalhost);
    std::size_t generateBiFlowHash(OSFingerprint* fp) const;
    ts_map tsMap;
    rr_map rrMap;

};

#endif
#endif
