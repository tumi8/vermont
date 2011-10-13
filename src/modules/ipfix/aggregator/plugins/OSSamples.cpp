#ifdef PLUGIN_SUPPORT_ENABLED
#include "OSSamples.h"
#include <arpa/inet.h>
using namespace std;

void OSSamples::addToSample(uint32_t ip, uint32_t timestamp, timeval capturetime, uint16_t ipid, uint32_t seq_number){
    if (tsMap.find(ip) == tsMap.end()) {
        tsMap[ip].index = 0;
    }

    if ( tsMap[ip].index < NUM_MAX_SAMPLES ){
        //add a new sample
        tsMap[ip].timestamps[tsMap[ip].index] = timestamp;
        tsMap[ip].capturetimes[tsMap[ip].index] = capturetime;
        tsMap[ip].ipids[tsMap[ip].index] = (int) ipid;
        tsMap[ip].seq_numbers[tsMap[ip].index] = seq_number;
        tsMap[ip].index++;
    } else {
        //predict TS Sequence type
        predictSequences(ip);
    }
}


/**
  * Based on NMAP T1_7 Tests (osscan2.cc)
  *
  **/
void OSSamples::predictSequences(uint32_t ip){
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
    int icmp_ipid_seqclass; /* ICMP IPID SEQ TYPE defines in nmap.h */
    int good_tcp_ipid_num;
    uint32_t ts_diffs[NUM_MAX_SAMPLES];
    uint32_t seq_rates[NUM_MAX_SAMPLES];
    uint32_t seq_diffs[NUM_MAX_SAMPLES];
    unsigned long time_usec_diffs[NUM_MAX_SAMPLES];
    for(i=1; i < NUM_MAX_SAMPLES; i++) {
        //calculate sequence differences
        seq_diffs[i - 1] = MOD_DIFF(tsMap[ip].seq_numbers[i], tsMap[ip].seq_numbers[i - 1]);
        //calculate timestamp differnces
        ts_diffs[i - 1] = MOD_DIFF(tsMap[ip].timestamps[i], tsMap[ip].timestamps[i-1]);
        //calculate time between timestamps
        time_usec_diffs[i - 1] = TIMEVAL_SUBTRACT(tsMap[ip].capturetimes[i], tsMap[ip].capturetimes[i-1]);
        if (!time_usec_diffs[i - 1]) time_usec_diffs[i - 1]++; /* We divide by this later */
        /* Rate of ISN increase per second */
        seq_rates[i - 1] = seq_diffs[i-1] * 1000000.0 / time_usec_diffs[i-1];
        seq_avg_rate += seq_rates[i-1];
    }

    /**
      * Begin IP ID prediction
      */

    seq_avg_rate /= NUM_MAX_SAMPLES - 1;
    seq_rate = seq_avg_rate;

    /* First calculate the GCD */
    seq_gcd = gcd_n_uint(NUM_MAX_SAMPLES -1, seq_diffs);

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

        for(i=0; i < NUM_MAX_SAMPLES - 1; i++) {
            double rtmp = seq_rates[i] / div_gcd - seq_avg_rate / div_gcd;
            seq_stddev += rtmp * rtmp;
        }

        /* We divide by ((numelements in seq_diffs) - 1), which is
         (NUM_MAX_SAMPLES - 2), because that gives a better approx of
         std. dev when you're only looking at a subset of whole
         population. */
        seq_stddev /= NUM_MAX_SAMPLES - 2;

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

//    AV.attribute = "SP";
//    AV.value = string_pool_sprintf("%X", hss->si.index);
//    seq_AVs.push_back(AV);
//    AV.attribute = "GCD";
//    AV.value = string_pool_sprintf("%X", seq_gcd);
//    seq_AVs.push_back(AV);
//    AV.attribute = "ISR";
//    AV.value = string_pool_sprintf("%X", (unsigned int) seq_rate);
//    seq_AVs.push_back(AV);

    printf("SP %i, GCD %i, ISR %i ", index, seq_gcd, (uint32_t) seq_rate);


    /* Now it is time to deal with IPIDs */
    good_tcp_ipid_num = 0;

    for(i=0; i < NUM_MAX_SAMPLES; i++) {
        if (tsMap[ip].ipids[i] != -1) {
            if (good_tcp_ipid_num < i) {
                tsMap[ip].ipids[good_tcp_ipid_num] = tsMap[ip].ipids[i];
            }
            good_tcp_ipid_num++;
        }
    }

    if (good_tcp_ipid_num >= 3) {
        tcp_ipid_seqclass = get_ipid_sequence(good_tcp_ipid_num, (int*) tsMap[ip].ipids, 0);
    } else {
        tcp_ipid_seqclass = IPID_SEQ_UNKNOWN;
    }

    printf("IPID_SEQC %i ", tcp_ipid_seqclass);


//    /* This fills in TI=Z or something like that. */
//    if (make_aval_ipid_seq(&AV, "TI", tcp_ipid_seqclass, hss->ipid.tcp_ipids) != NULL)
//        seq_AVs.push_back(AV);
//    if (make_aval_ipid_seq(&AV, "CI", tcp_closed_ipid_seqclass, hss->ipid.tcp_closed_ipids) != NULL)
//        seq_AVs.push_back(AV);
//    if (make_aval_ipid_seq(&AV, "II", icmp_ipid_seqclass, hss->ipid.icmp_ipids) != NULL)
//        seq_AVs.push_back(AV);

    /* SS: Shared IP ID sequence boolean */
    if ( (tcp_ipid_seqclass == IPID_SEQ_INCR ||
          tcp_ipid_seqclass == IPID_SEQ_BROKEN_INCR ||
          tcp_ipid_seqclass == IPID_SEQ_RPI) &&
         (icmp_ipid_seqclass == IPID_SEQ_INCR ||
          icmp_ipid_seqclass == IPID_SEQ_BROKEN_INCR ||
          icmp_ipid_seqclass == IPID_SEQ_RPI)) {
        /* Both are incremental. Thus we have "SS" test. Check if they
       are in the same sequence. */
//        AV.attribute = "SS";
        int avg = (tsMap[ip].ipids[good_tcp_ipid_num-1] - tsMap[ip].ipids[0]) / (good_tcp_ipid_num - 1);
        if ( tsMap[ip].ipids[0] < tsMap[ip].ipids[good_tcp_ipid_num-1] + 3 * avg) {
//            AV.value = "S";
        } else {
//            AV.value = "O";
        }
    }

    /**
      * Begin TCP Sequence prediction
      */

    uint8_t ts_seqclass = TS_SEQ_UNKNOWN;
    avg_ts_hz = 0.0;
    for(i=0; i < NUM_MAX_SAMPLES - 1; i++) {
        double dhz;

        dhz = (double) ts_diffs[i] / (time_usec_diffs[i] / 1000000.0);
        /*       printf("ts incremented by %d in %li usec -- %fHZ\n", ts_diffs[i], time_usec_diffs[i], dhz); */
        avg_ts_hz += dhz / (NUM_MAX_SAMPLES - 1);
    }

    if (avg_ts_hz > 0 && avg_ts_hz < 5.66) { /* relatively wide range because sampling time so short and frequency so slow */
        ts_seqclass = TS_SEQ_2HZ;
    }
    else if (avg_ts_hz > 70 && avg_ts_hz < 150) {
        ts_seqclass = TS_SEQ_100HZ;
    }
    //added this group as values in this region appear to show up often
    else if (avg_ts_hz > 200 && avg_ts_hz < 400) {
        ts_seqclass = TS_SEQ_300HZ;
    }
    else if (avg_ts_hz > 724 && avg_ts_hz < 1448) {
        ts_seqclass = TS_SEQ_1000HZ;
    }
    else if (avg_ts_hz > 0) {
        ts_seqclass = TS_SEQ_OTHER_NUM;
    }
    struct in_addr sip;
    sip.s_addr = (in_addr_t) ip;
    printf("SEQ %i AVG %f IP %s\n", ts_seqclass, avg_ts_hz, inet_ntoa(sip));
    tsMap.erase(ip);
}

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
