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
#include "OSResultAggregator.h"
#include <stdio.h>
#include <arpa/inet.h>
#include "common/Time.h"
#include <time.h>
#include <errno.h>
using namespace std;

typedef boost::unordered_map< OSDetail, int > os_count_map_t;

OSResultAggregator::OSResultAggregator()
    :thread(OSResultAggregator::threadWrapper), pollInterval(1800000), exitFlag(false)
{
    thread.run(this);
}

OSResultAggregator::~OSResultAggregator(){
    exitFlag = true;
    thread.join();
}

/**
 * thread which regularly checks for expired results
 */
void OSResultAggregator::exporterThread()
{
    struct timeval inttimer;
    gettimeofday(&inttimer, 0);
    while (!exitFlag) {
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
        DPRINTFL(MSG_VDEBUG,"OSResultAggregator: starting Export");

        os_result_map_t::iterator itBegin = results.begin();
        os_result_map_t::iterator itEnd   = results.end();
        os_result_map_t::iterator itTemp;
        // while loop used to be able to delete while iterating
        while (itBegin != itEnd)
        {
            //print and delete all expired results
            uint32_t now = time(0);
            if (expired[itBegin->first] < now)
            {
                //reset expiry
                expired.erase(itBegin->first);
                //output statistics
                analyseResults(itBegin->first);
                itTemp = itBegin;
                ++itBegin;
                //reset result
                results.erase(itTemp);
            } else {
                ++itBegin;
            }
        }

        struct timeval endtime;
        gettimeofday(&endtime, 0);
        timeval_subtract(&difftime, &endtime, &curtime);

        DPRINTFL(MSG_VDEBUG,"OSResultAggregator: export took %.03f secs", (float)difftime.tv_usec/1000000+difftime.tv_sec);
    }
}

void* OSResultAggregator::threadWrapper(void* instance){
    OSResultAggregator* osa = reinterpret_cast<OSResultAggregator*>(instance);
    osa->exporterThread();
    return 0;
}

/**
  * Add new OSDetails to the list of results
  **/
void OSResultAggregator::insertResult(uint32_t ip, OSDetail details){
    if (results.find(ip) == results.end()){
        results[ip] = list<OSDetail>();
        expired[ip] = time(0);
    }
    results[ip].push_back(details);
}

/**
  * Remove a all results of a given IP, e.g. if a timer has expired
  **/
void OSResultAggregator::removeResult(uint32_t ip){
    if (results.find(ip) != results.end()) results.erase(ip);
}

/**
  * Analyse all OSDetails of a given IP
  **/
void OSResultAggregator::analyseResults(uint32_t ip){
    if (results.find(ip) == results.end()) return;
    OSDetail detail;
    os_count_map_t map;

    list<OSDetail> detailsList = (std::list<OSDetail>) results[ip];

    // group by OSDetail and count results
    for (list<OSDetail>::iterator it = detailsList.begin(); it != detailsList.end(); it++){
        detail = (OSDetail) (*it);
        if (map.find(detail) == map.end()) map[detail] = 0;
        map[detail]++;
    }

    //calculate number of results in total
    uint32_t total_results = 0;
    for (os_count_map_t::iterator it = map.begin(); it != map.end(); it++){
        total_results += it->second;
    }
    //print statistics
    struct in_addr saddr;
    saddr.s_addr = ip;

    printf("------------------------------------------\n");
    printf("STATISTICS for IP: %s\n\n", inet_ntoa(saddr));
    for (os_count_map_t::iterator it = map.begin(); it != map.end(); it++){
        detail = (OSDetail)(it->first);
        printf("OS detected via: %s (%i)\n", detail.origin == FINGERPRINT ? "Fingerprint" : "Banner Grabbing", total_results);
        printf("OS Type: %s\n", detail.os_type.c_str());
        printf("OS Version: %s\n", detail.os_version.c_str());
        printf("OS Architecture: %s\n", detail.architecture.c_str());
        printf("OS Success Ratio: %g%\n\n", 100 * (map[detail] / (double) total_results));
    }
    printf("------------------------------------------\n");
}
#endif
