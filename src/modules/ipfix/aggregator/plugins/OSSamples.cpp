#ifdef PLUGIN_SUPPORT_ENABLED
#include "OSSamples.h"
#include "OSFingerprint.h"
#include <arpa/inet.h>
using namespace std;


/**
  * add a new sample to the hash map and start prediction if num_samples is
  * reached
  */
void OSSamples::addToSample(uint32_t hash, OSFingerprint* fp, timeval capturetime){
    // grab data for request/response test
//    if (fp->m_TCP_SYN || (fp->m_TCP_SYN && fp->m_TCP_ACK)){
//        std::size_t biFlowHash = generateBiFlowHash(fp);

//        if (rrMap.find(biFlowHash) == rrMap.end()) {
//            rrMap[biFlowHash].syn_seq_number = 0;
//            rrMap[biFlowHash].syn_ack_seq_number = 0;
//        }

//        rrMap[biFlowHash].syn_seq_number = (fp->m_TCP_SYN && !fp->m_TCP_ACK) ? fp->m_TCP_SEQ : rrMap[biFlowHash].syn_seq_number;
//        rrMap[biFlowHash].syn_ack_seq_number = (fp->m_TCP_SYN && fp->m_TCP_ACK) ? fp->m_TCP_ACK_SEQ : rrMap[biFlowHash].syn_ack_seq_number;

//        if (rrMap[biFlowHash].syn_seq_number > 0 && rrMap[biFlowHash].syn_ack_seq_number > 0) {
//            int seq_diff = rrMap[biFlowHash].syn_ack_seq_number - rrMap[biFlowHash].syn_seq_number;
//            printf("\nSEQUENCE FOUND; DIFF IS: %u for IP: %s\n", seq_diff, inet_ntoa(fp->m_IP_Source));
//            rrMap.erase(biFlowHash);
//        }

//    }

    // only process SYN/ACK packets
    if (!fp->m_TCP_SYN) return;

    uint32_t ip = fp->m_IP_Source.s_addr;

    if (tsMap.find(ip) == tsMap.end()) {
        tsMap[ip].index = 0;
    }

    if ( tsMap[ip].index < NUM_MAX_SAMPLES ){
        //add a new sample
        tsMap[ip].timestamps[tsMap[ip].index] = fp->m_TCP_Options.timestamp_set ? (int) fp->m_TCP_Options.tstamp : -1;
        tsMap[ip].capturetimes[tsMap[ip].index] = capturetime;
        tsMap[ip].ipids[tsMap[ip].index] = (int) fp->m_IP_ID;
        tsMap[ip].seq_numbers[tsMap[ip].index] = fp->m_TCP_SEQ;
        tsMap[ip].ips.insert(ip);
        tsMap[ip].index++;
    } else {
        //predict TS Sequence type
        predictSequences(ip, ip);
    }
}

/**
  * force prediction if e.g. bucket[hash] is going to be deleted.
  * this might be neccessary if one would like to assure, that only
  * results of a single connection are taken into account
  **/
void OSSamples::forceOSSamplePrediction(uint32_t hash) {
    uint32_t ip;
    // only start prediction if ip, hash pair is unique
    if (int (tsMap[hash].ips.size()) == 1){
        ip = *tsMap[hash].ips.begin();
        predictSequences(ip, hash);
    }
}

std::size_t OSSamples::generateBiFlowHash(OSFingerprint* fp) const {
    in_addr_t ip1;
    u_int16_t port1;
    in_addr_t ip2;
    u_int16_t port2;

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
  * Based on NMAP T1_7 Tests (osscan2.cc)
  *
  **/
void OSSamples::predictSequences(uint32_t ip, uint32_t hash){
    /* Now we look at TCP Timestamp sequence prediction */
    /* Battle plan:
         1) Compute average increments per second, and variance in incr. per second
         2) If any are 0, set to constant
         3) If variance is high, set to random incr. [ skip for now ]
         4) if ~10/second, set to appropriate thing
         5) Same with ~100/sec
      */
    int i;
    double seq_stddev = 0;
    double seq_rate = 0;
    double seq_avg_rate = 0;
    double avg_ts_hz = 0.0; /* Avg. amount that timestamps incr. each second */
    int index;
    uint32_t seq_gcd = 1;
    int tcp_ipid_seqclass; /* TCP IPID SEQ TYPE defines in nmap.h */
    int good_tcp_ipid_num;
    int good_tcp_ts_num;
    int num_samples = (int)tsMap[hash].index;
    uint32_t ts_diffs[num_samples];
    uint32_t seq_rates[num_samples];
    uint32_t seq_diffs[num_samples];
    unsigned long time_usec_diffs[num_samples];
    for(i=1; i < num_samples; i++) {
        //calculate sequence differences
        seq_diffs[i - 1] = MOD_DIFF(tsMap[hash].seq_numbers[i], tsMap[hash].seq_numbers[i - 1]);
        /* Rate of ISN increase per second */
        seq_rates[i - 1] = seq_diffs[i-1] * 1000000.0 / time_usec_diffs[i-1];
        seq_avg_rate += seq_rates[i-1];
    }

    //only use good timestamps
    good_tcp_ts_num = 0;
    for(i=0; i < num_samples; i++) {
        if (tsMap[hash].timestamps[i] != -1) {
            if (good_tcp_ts_num < i) {
                tsMap[hash].timestamps[good_tcp_ts_num] = tsMap[hash].timestamps[i];
                tsMap[hash].capturetimes[good_tcp_ts_num] = tsMap[hash].capturetimes[i];
            }
            good_tcp_ts_num++;
        }
    }
    for(i=1; i < good_tcp_ts_num; i++) {
        //calculate timestamp differnces
        ts_diffs[i - 1] = MOD_DIFF(tsMap[hash].timestamps[i], tsMap[hash].timestamps[i-1]);
        //calculate time between timestamps
        time_usec_diffs[i - 1] = TIMEVAL_SUBTRACT(tsMap[hash].capturetimes[i], tsMap[hash].capturetimes[i-1]);
        if (!time_usec_diffs[i - 1]) time_usec_diffs[i - 1]++; /* We divide by this later */
    }
    /**
      * Begin IP ID prediction
      */

    seq_avg_rate /= num_samples - 1;
    seq_rate = seq_avg_rate;

    /* First calculate the GCD */
    seq_gcd = gcd_n_uint(num_samples -1, seq_diffs);

    if (!seq_gcd) {
        /* Constant ISN */
        seq_rate = 0;
        seq_stddev = 0;
        index = 0;
    } else {

        /* Finally we take a binary logarithm, multiply by 8, and round
         to get the final result */
        seq_rate = log(seq_rate) / log(2.0);
        seq_rate = (uint32_t) (seq_rate * 8 + 0.5);

        /* Normally we don't divide by gcd in computing the rate stddev
         because otherwise we'll get an artificially low value about
         1/32 of the time if the responses all happen to be even.  On
         the other hand, if a system inherently uses a large gcd such
         as 64,000, we want to get rid of it.  So as a compromise, we
         divide by the gcd if it is at least 9 */

        int div_gcd = 1;
        if (seq_gcd > 9)
            div_gcd = seq_gcd;

        for(i=0; i < num_samples - 1; i++) {
            double rtmp = seq_rates[i] / div_gcd - seq_avg_rate / div_gcd;
            seq_stddev += rtmp * rtmp;
        }

        /* We divide by ((numelements in seq_diffs) - 1), which is
         (num_samples - 2), because that gives a better approx of
         std. dev when you're only looking at a subset of whole
         population. */
        seq_stddev /= num_samples - 2;

        /* Next we need to take the square root of this value */
        seq_stddev = sqrt(seq_stddev);

        /* Finally we take a binary logarithm, multiply by 8, and round
         to get the final result */
        if (seq_stddev <= 1)
            index = 0;
        else {
            seq_stddev = log(seq_stddev) / log(2.0);
            index = (int) (seq_stddev * 8 + 0.5);
        }
    }
    /* Variables: TCP ISN greatest common divisor (GCD), TCP ISN sequence predictability index (SP),
        TCP ISN counter rate (ISR)*/
    printf("SP %i, GCD %i, ISR %i \n", index, seq_gcd, (uint32_t) seq_rate);


    /* Now it is time to deal with IPIDs */
    good_tcp_ipid_num = 0;

    for(i=0; i < num_samples; i++) {
        if (tsMap[hash].ipids[i] != -1) {
            if (good_tcp_ipid_num < i) {
                tsMap[hash].ipids[good_tcp_ipid_num] = tsMap[hash].ipids[i];
            }
            good_tcp_ipid_num++;
        }
    }

    if (good_tcp_ipid_num >= 3) {
        tcp_ipid_seqclass = get_ipid_sequence(good_tcp_ipid_num, (int*) tsMap[hash].ipids, 0);
    } else {
        tcp_ipid_seqclass = IPID_SEQ_UNKNOWN;
    }

    printf("IPID_SEQC %i \n", tcp_ipid_seqclass);

    /**
      * Begin TCP Sequence prediction
      */


    uint8_t ts_seqclass = TS_SEQ_UNKNOWN;
    avg_ts_hz = 0.0;
    for(i=0; i < good_tcp_ts_num - 1; i++) {
        double dhz;

        dhz = (double) ts_diffs[i] / (time_usec_diffs[i] / 1000000.0);
        /*       printf("ts incremented by %d in %li usec -- %fHZ\n", ts_diffs[i], time_usec_diffs[i], dhz); */
        avg_ts_hz += dhz / (num_samples - 1);
    }

    if (avg_ts_hz > 0 && avg_ts_hz < 5.66) { /* relatively wide range because sampling time so short and frequency so slow */
        ts_seqclass = TS_SEQ_2HZ;
    }
    else if (avg_ts_hz > 70 && avg_ts_hz < 150) {
        ts_seqclass = TS_SEQ_100HZ;
    }
    else if (avg_ts_hz > 724 && avg_ts_hz < 1448) {
        ts_seqclass = TS_SEQ_1000HZ;
    }
    else if (avg_ts_hz > 0) {
        ts_seqclass = TS_SEQ_OTHER_NUM;
    }
    struct in_addr sip;
    sip.s_addr = (in_addr_t) ip;
    printf("SEQ %i AVG %f IP %s\n\n", ts_seqclass, avg_ts_hz, inet_ntoa(sip));

    if (ts_seqclass == TS_SEQ_UNKNOWN && avg_ts_hz == 0.0){
        printf("--> Must be Windows\n\n");
    }
    tsMap.erase(hash);
}

/**
  * Taken from nmap
  **/
uint32_t OSSamples::gcd_n_uint(int nvals, unsigned int *val) {
    uint32_t a,b,c;

    if (!nvals) return 1;
    a=*val;
    for (nvals--;nvals;nvals--)
    {
        b=*++val;
        if (a<b) { c=a; a=b; b=c; }
        while (b) { c=a%b; a=b; b=c; }
    }
    return a;
}

/**
  * Taken from nmap
  **/
/* This function takes an array of "numSamples" IP IDs and analyzes
 them to determine their sequenceability classification.  It returns
 one of the IPID_SEQ_* classifications defined in nmap.h .  If the
 function cannot determine the sequence, IPID_SEQ_UNKNOWN is returned.
 This islocalhost argument is a boolean specifying whether these
 numbers were generated by scanning localhost.  NOTE: the "ipids" argument
 may be modified if localhost is set to true. */

int OSSamples::get_ipid_sequence(int numSamples, int *ipids, int islocalhost) {
    uint16_t ipid_diffs[32];
    int i;
    int allipideqz = 1;  /* Flag that means "All IP.IDs returned during
                                                  sequencing are zero.  This is unset if we
                                                  find a nonzero */
    int j,k;

    if (numSamples < 2) return IPID_SEQ_UNKNOWN;

    for(i = 1; i < numSamples; i++) {
        if (ipids[i-1] != 0 || ipids[i] != 0)
            allipideqz = 0; /* All IP.ID values do *NOT* equal zero */

        if (ipids[i-1] <= ipids[i]) {
            ipid_diffs[i-1] = ipids[i] - ipids[i-1];
        } else {
            ipid_diffs[i-1] = (uint16_t) (ipids[i] - ipids[i-1] + 65536);
        }

        /* Random */
        if (numSamples > 2 && ipid_diffs[i-1] > 20000)
            return IPID_SEQ_RD;
    }

    /* ZERO */
    if (allipideqz) return IPID_SEQ_ZERO;

    if (islocalhost) {
        int allgto = 1; /* ALL diffs greater than one */

        for(i=0; i < numSamples - 1; i++) {
            if (ipid_diffs[i] < 2) {
                allgto = 0; break;
            }
        }

        if (allgto) {
            for(i=0; i < numSamples - 1; i++) {
                if (ipid_diffs[i] % 256 == 0) /* Stupid MS */
                    ipid_diffs[i] -= 256;
                else
                    ipid_diffs[i]--; /* Because on localhost the RST sent back use an IPID */
            }
        }
    }

    /* Constant */
    j = 1; /* j is a flag meaning "all differences seen are zero" */
    for(i=0; i < numSamples - 1; i++) {
        if (ipid_diffs[i] != 0) {
            j = 0;
            break;
        }
    }
    if (j) {
        return IPID_SEQ_CONSTANT;
    }

    /* Random Positive Increments */
    for(i=0; i < numSamples - 1; i++) {
        if (ipid_diffs[i] > 1000 &&
                (ipid_diffs[i] % 256 != 0 ||
                 (ipid_diffs[i] % 256 == 0 && ipid_diffs[i] >= 25600))) {
            return IPID_SEQ_RPI;
        }
    }

    j = 1; /* j is a flag meaning "all differences seen are < 10" */
    k = 1; /* k is a flag meaning "all difference seen are multiples of
                        256 and no greater than 5120" */
    for(i=0; i < numSamples - 1; i++) {
        if (k && (ipid_diffs[i] > 5120 || ipid_diffs[i] % 256 != 0)) {
            k = 0;
        }

        if (j && ipid_diffs[i] > 9) {
            j = 0;
        }
    }

    /* Broken Increment */
    if (k == 1) {
        return IPID_SEQ_BROKEN_INCR;
    }

    /* Incremental */
    if (j == 1)
        return IPID_SEQ_INCR;

    return IPID_SEQ_UNKNOWN;
}

#endif
