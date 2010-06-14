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
#include <iostream>
#include <fstream>
#include <map>

InstanceManager<IDMEFMessage> AnomalyDetector::idmefManager("AnomalyDetectorIDMEFMessage",0);


/**
 * Constructor
 */ 
AnomalyDetector::AnomalyDetector(uint32_t subnet, uint32_t subnetmask, double packetRateThreshold, string analyzerid, string idmeftemplate)
	:subnet(subnet),
	 subnetmask(subnetmask),
   packetRateThreshold(packetRateThreshold),
   analyzerId(analyzerid),
	 idmefTemplate(idmeftemplate)
{
    alpha = 0.3;  // smoothing factor for EMA
    //subnetmask = 0;
    
    // print headers to logfile
    ofstream logfile;
    logfile.open("AnomalyDetector.txt");
  
    if (logfile.is_open())
    {
        // write headings
        logfile << "time" << "\t";
        logfile << "sourceIPAddress" << "\t";
        logfile << "packetRate" << "\t";
        logfile << "estimate (ema)" << "\t";
        logfile << "over threshold" << "\t";
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
    emaMap.erase(emaMap.begin(), emaMap.end());
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
    
    // only use this connection if it is from local host
    if ((conn.srcIP & subnetmask) == subnet) {
        checkConnection(&conn);
    }
    
    // cleanupEntries();
    record->removeReference();
}


/**
 * Check if connection is an anomaly
 */
void AnomalyDetector::checkConnection(Connection* conn)
{
    float ema;          // estimate for current packetrate (T)
    float newEma;       // new estimate for packetrate at T+1
    float packetRate;   // packetRate
    float flowLength;   // length of current flow (milliseconds)
    
    flowLength = conn->srcTimeEnd - conn->srcTimeStart;
    
    if (flowLength != 0) {
      packetRate = ntohll(conn->srcPackets) / flowLength;
    } else {
      packetRate = 0;
    }
    
    // get last calculated ema for IP
    map<uint32_t, float>::iterator it;
    it = emaMap.find(conn->srcIP);

    if(it != emaMap.end())
    {
      // srcIP already in map -> get ema for T
      ema = emaMap[conn->srcIP];
      // calculate ema for T+1
      newEma = alpha * packetRate + (1 - alpha) * ema;
      // update map
      emaMap[conn->srcIP] = newEma;
      
    } else {
      // calculate initial ema with packetRate
      newEma = alpha * packetRate + (1 - alpha) * packetRate;
      // insert new entry in map
      emaMap.insert(pair<uint32_t, float>(conn->srcIP, newEma));
    }

    // check for anomaly using ema for time T
    if ((packetRate - ema) > packetRateThreshold) {
        // anomaly found -> print stuff to file
        ofstream logfile;
        logfile.open("AnomalyDetector.txt", ios::out | ios::app );
        if (logfile.is_open()) {
            logfile << conn->srcTimeStart << "\t";
            logfile << IPToString(conn->srcIP).c_str() << "\t";
            logfile << packetRate << "\t";
            logfile << ema << "\t";
            logfile << (packetRate - ema) - packetRateThreshold << "\t";
            logfile << endl;
            logfile.close();
        }
        else cout << "Unable to open file";
    }
}