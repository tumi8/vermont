/* VERMONT 
 * Copyright (C) 2010 Manuela Schinn <manuela.schinn@gmx.de>
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
 */

#include "AnomalyDetector.h"
#include "common/crc.hpp"
#include "common/Misc.h"
#include <arpa/inet.h>
#include <math.h>
#include <ctime>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <map>

InstanceManager<IDMEFMessage> AnomalyDetector::idmefManager("AnomalyDetectorIDMEFMessage",0);

/**
 * Constructor
 */ 
AnomalyDetector::AnomalyDetector(uint32_t subnet, uint32_t subnetmask, double packetRateThreshold, double alpha, string analyzerid, string idmeftemplate)
	:subnet(subnet),
	 subnetmask(subnetmask),
   packetRateThreshold(packetRateThreshold),
   alpha(alpha),
   analyzerId(analyzerid),
	 idmefTemplate(idmeftemplate)
{   
    // print headers to logfile
    ofstream logfile;
    logfile.open("anomDetectAlert");
  
    if (logfile.is_open())
    {
        // write headings
        logfile << "src/dst" << "\t";
        logfile << "host" << "\t";
        logfile << "flowStartMillisec" << "\t";
        logfile << "binVal" << "\t";
        logfile << "packetRate" << "\t";
        logfile << "ema" << "\t";
        logfile << endl;
        logfile.close();
    }
    else cout << "Unable to open file";
}


/**
 * Destructor
 */ 
AnomalyDetector::~AnomalyDetector()
{
    srcHostMap.erase(srcHostMap.begin(), srcHostMap.end());
    dstHostMap.erase(dstHostMap.begin(), dstHostMap.end());
}


/**
 * Convert IPFIXRecord to Connection Struct
 */
void AnomalyDetector::onDataRecord(IpfixDataRecord* record)
{
    // only treat non-Options Data Records (although we cannot be sure that there is a Flow inside)
    if((record->templateInfo->setId != TemplateInfo::NetflowTemplate) 
        && (record->templateInfo->setId != TemplateInfo::IpfixTemplate) 
        && (record->templateInfo->setId != TemplateInfo::IpfixDataTemplate)) {
        record->removeReference();
        return;
    }
    
    // convert ipfixrecord to connection struct
    Connection conn(record);
    
    // check if connection is an anomaly
    checkConnection(&conn);
    
    // cleanupEntries();
    record->removeReference();
}


/**
 * Check if connection is an anomaly
 */
void AnomalyDetector::checkConnection(Connection* conn)
{
    float numFlowPackets;           // number of sent or received packets for current flow
    float ivLength = 1;             // length of observation interval(default = 1 sek)
    uint32_t flowStartSec = 0;      // starttime of flow (seconds)
    uint64_t flowStartMillisec = 0; // starttime of flow (milliseconds)
    uint32_t host = 0;              // host in local network (srcIP or dstIP)
    map<uint32_t, EmaEntry> *emaMap;
    bool isSrc = false;
    
    // check if src or dst of current flow is local host
    if ((conn->srcIP & subnetmask) == subnet) {
        host = conn->srcIP;
        emaMap = &srcHostMap; // Outgoing Map
        isSrc = true;
        
    } else if ((conn->dstIP & subnetmask) == subnet) {
        host = conn->dstIP;
        emaMap = &dstHostMap; // Incoming Map
        isSrc = false;
    } else {
        return; // ignore this connection
    }
    
    // get number of packets for current flow
    numFlowPackets = ntohll(conn->srcPackets);
    
    // calc starttime for current flow
    flowStartMillisec = conn->srcTimeStart;
    flowStartSec = (flowStartMillisec + 500) / 1000; // srcTimeStart -rounded- to seconds
    
    // look up host in map
    map<uint32_t, EmaEntry>::iterator it;
    it = (*emaMap).find(host);
    
    if(it != (*emaMap).end())
    {   
        // host already in map 
        if (flowStartSec < ((*emaMap)[host].binVal + ivLength)) {
            // flow is still within current interval -> just add packets
            (*emaMap)[host].numPackets += numFlowPackets;
            
        } else {
            (*emaMap)[host].flowStartMillisec = flowStartMillisec;
            // flow exceeded interval
            if (flowStartSec > (((*emaMap)[host].binVal) + (ivLength *2 ))) {
                // nothing happened for a while
                
                uint32_t timeSinceLastBin = flowStartSec - ((*emaMap)[host].binVal);
                uint32_t numIvSinceLastBin = timeSinceLastBin / ivLength;
                uint32_t newBinVal = (*emaMap)[host].binVal + (numIvSinceLastBin * ivLength);

                // calculate and save ema for last binVal interval
                (*emaMap)[host].packetRate = (*emaMap)[host].numPackets / ivLength;
                (*emaMap)[host].ema = alpha * ((*emaMap)[host].packetRate) + (1 - alpha) * ((*emaMap)[host].ema);
                // for testing purpose:
                // printEntry((*emaMap)[host], host);
                numIvSinceLastBin --;  // we just calculated the first interval
                
                // calculate ema for remaining intervals where no packets where transmitted
                while (numIvSinceLastBin != 0) {
                    (*emaMap)[host].ema = (1 - alpha) * ((*emaMap)[host].ema);
                    (*emaMap)[host].packetRate = 0;
                    (*emaMap)[host].binVal += ivLength;
                    numIvSinceLastBin--;
                }
                // for testing purpose:
                // printEntry((*emaMap)[host], host);
                
                // save current number of packets and current binVal
                (*emaMap)[host].numPackets = numFlowPackets;
                (*emaMap)[host].packetRate = numFlowPackets / ivLength;
                (*emaMap)[host].binVal = newBinVal;
                
            } else {
                // calculate and save ema for last binVal
                (*emaMap)[host].ema = alpha * ((*emaMap)[host].packetRate) + (1 - alpha) * ((*emaMap)[host].ema);
            
                // check for anomaly
                if ((((*emaMap)[host].packetRate) - ((*emaMap)[host].ema)) > packetRateThreshold) {
                    // anomaly found -> print stuff to file
                    printEntry((*emaMap)[host], host);
                }
            
                // update binVal with current binVal
                (*emaMap)[host].binVal += ivLength;
                // set number of packets for current binVal
                (*emaMap)[host].numPackets = numFlowPackets;
                (*emaMap)[host].packetRate = ((*emaMap)[host].numPackets) / ivLength;
              }
          }
    } else {
        // host not yet in map -> create new entry
        EmaEntry emaEnt;
        emaEnt.binVal = flowStartSec;
        emaEnt.numPackets = numFlowPackets;
        emaEnt.packetRate = numFlowPackets / ivLength;
        emaEnt.flowStartMillisec = flowStartMillisec;
        emaEnt.isSrc = isSrc;
        
        // set initial ema to current packetRate
        emaEnt.ema = alpha * emaEnt.packetRate;
        
        // insert new entry in map
        (*emaMap).insert(pair<uint32_t, EmaEntry>(host, emaEnt));
        
        // for testing purpose:
        // printEntry(emaEnt, host);
    }
}

    
void AnomalyDetector::printEntry(EmaEntry entry, uint32_t host)
{
    ofstream logfile;
    logfile.open("anomDetectAlert", ios::out | ios::app );
    if (logfile.is_open()) {
        if (entry.isSrc) {
             logfile << "SRC" << "\t";
        } else {
             logfile << "DST" << "\t";
        }
        logfile << IPToString(host).c_str() << "\t";
        logfile << entry.flowStartMillisec << "\t";
        logfile << entry.binVal << "\t";
        logfile << entry.packetRate << "\t";
        logfile << entry.ema << "\t";
        logfile << endl;
        logfile.close();
        } else {
             cout << "Unable to open file";
        }
}