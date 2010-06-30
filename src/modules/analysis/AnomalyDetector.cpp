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
AnomalyDetector::AnomalyDetector(uint32_t subnet, uint32_t subnetmask, double packetRateThreshold, double emaTimeout, string analyzerid, string idmeftemplate)
	:subnet(subnet),
	 subnetmask(subnetmask),
   packetRateThreshold(packetRateThreshold),
   emaTimeout(emaTimeout),
   analyzerId(analyzerid),
	 idmefTemplate(idmeftemplate)
{
    alpha = 0.3;  // smoothing factor for EMA
    
    // print headers to logfile
    ofstream logfile;
    logfile.open("anomDetectAlert");
  
    if (logfile.is_open())
    {
        // write headings
        logfile << "src/dst" << "\t";
        logfile << "host" << "\t";
        logfile << "flowStart" << "\t";
        logfile << "flowBinVal" << "\t";
        logfile << "packetRate" << "\t";
        logfile << "ema" << "\t";
        logfile << "newEma" << "\t";
        logfile << "threshold" << "\t";
        logfile << "overThreshold" << "\t";
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
    
    checkConnection(&conn);
    
    // cleanupEntries();
    record->removeReference();
}


/**
 * Check if connection is an anomaly
 */
void AnomalyDetector::checkConnection(Connection* conn)
{
    float numFlowPackets;   // number of sent or received packets for current flow
    float packetRate;
    float ema;
    float newEma;
    float ivLength = 1;     // length of observation interval(default = 1 sek)
    uint32_t flowBinVal;    // bin value for current flow
    uint32_t host = 0;      // host in local network (srcIP or dstIP)
    map<uint32_t, EmaEntry> *emaMap;
    bool isSrc = false;
    
    ofstream testfile;
    testfile.open("anomTest", ios::out | ios::app );
    
    // check if src or dst of current flow is local host
    if ((conn->srcIP & subnetmask) == subnet) {
        host = conn->srcIP;
        emaMap = &srcHostMap; // Outgoing Map
        isSrc = true;
        
    } else if ((conn->dstIP & subnetmask) == subnet) {
        host = conn->dstIP;
        emaMap = &dstHostMap; // Incoming Map
    }
    
    // get number of Packets for current flow
    numFlowPackets = ntohll(conn->srcPackets);
    
    // calc bin value for current flow
    flowBinVal = (conn->srcTimeStart + 500) / 1000; // srcTimeStart -rounded- to seconds
    
    // look up host in map
    map<uint32_t, EmaEntry>::iterator it;
    it = (*emaMap).find(host);
    
    if(it != (*emaMap).end())
    {
        // host already in map 
        if (flowBinVal == (*emaMap)[host].binVal) {
            // current binVal already exists -> just add packets
            (*emaMap)[host].numPackets += numFlowPackets;
        
        } else {
            if (flowBinVal > (((*emaMap)[host].binVal) + emaTimeout)) {
                // nothing happened for a long time -> don't use old ema value in map
                
                // update binVal with current binVal
                (*emaMap)[host].binVal = flowBinVal;
                
                // set number of packets for current binVal
                (*emaMap)[host].numPackets = numFlowPackets;
                
                // calculate initial ema with packetRate
                packetRate = numFlowPackets / ivLength;
                newEma = alpha * packetRate + (1 - alpha) * packetRate;
                (*emaMap)[host].ema = newEma;
                
            } else {
                // calc and save ema for last binVal
                
                packetRate = (*emaMap)[host].numPackets / ivLength;            
                ema = (*emaMap)[host].ema;
                newEma = alpha * packetRate + (1 - alpha) * ema;  // calc new ema
                (*emaMap)[host].ema = newEma; // update map with new ema
            
                // check for anomaly
                if ((packetRate - newEma) > packetRateThreshold) {
                    // anomaly found -> print stuff to file
                    ofstream logfile;
                    logfile.open("anomDetectAlert", ios::out | ios::app );
                        if (logfile.is_open()) {
                            if (isSrc) {
                              logfile << "SRC" << "\t";
                            } else {
                              logfile << "DST" << "\t";
                            }
                            logfile << IPToString(host).c_str() << "\t";
                            logfile << conn->srcTimeStart << "\t";
                            logfile << flowBinVal << "\t";
                            logfile << packetRate << "\t";
                            logfile << ema << "\t";
                            logfile << newEma << "\t";
                            logfile << packetRateThreshold << "\t";
                            logfile << (packetRate - newEma) - packetRateThreshold << "\t";
                            logfile << endl;
                            logfile.close();
                        } else {
                            cout << "Unable to open file";
                        }
                }
            
                // update binVal with current binVal
                (*emaMap)[host].binVal = flowBinVal;
                // set number of packets for current binVal
                (*emaMap)[host].numPackets = numFlowPackets;
              }
          }
    } else {
        // host not yet in map -> create new entry
        EmaEntry emaEnt;
        emaEnt.binVal = flowBinVal;
        emaEnt.numPackets = numFlowPackets;
        
        // calculate initial ema with packetRate
        packetRate = numFlowPackets / ivLength;
        emaEnt.ema = alpha * packetRate + (1 - alpha) * packetRate;
        
        // insert new entry in map
        (*emaMap).insert(pair<uint32_t, EmaEntry>(host, emaEnt));
    }
}