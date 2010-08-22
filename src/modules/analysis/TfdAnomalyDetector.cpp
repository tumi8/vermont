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
#include "TfdAnomalyDetector.h"
#include "common/crc.hpp"
#include "common/Misc.h"
#include <arpa/inet.h>
#include <math.h>
#include <ctime>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <map>
#include <algorithm>
#include <vector>

InstanceManager<IDMEFMessage> TfdAnomalyDetector::idmefManager("TfdAnomalyDetectorIDMEFMessage",0);

/**
 * Constructor
 */ 
TfdAnomalyDetector::TfdAnomalyDetector(uint32_t subnet, uint32_t subnetmask, double tfdThreshold, double ivLength, string analyzerid, string idmeftemplate)
	:subnet(subnet),
	 subnetmask(subnetmask),
	 tfdThreshold(tfdThreshold),
	 ivLength(ivLength),
   analyzerId(analyzerid),
	 idmefTemplate(idmeftemplate)
{   
    // init all arrays with 0
    for(int i=0; i < TIMEBINS; i++) {
      for(int j=0; j < OD_PAIRS; j++) {
      
        // temp arrays
        srcIpPackets[i][j] = 0;
        dstIpPackets[i][j] = 0;
        srcPortNum[i][j] = 0;
        dstPortNum[i][j] = 0;
        
        // entropy arrays
        srcIpEntropy[i][j] = 0.0;
        dstIpEntropy[i][j] = 0.0;
        srcPortEntropy[i][j] = 0.0;
        dstPortEntropy[i][j] = 0.0;
      }
    }
    
    lastFlowStartSec = 0;
    timeBin = 0;

    // print headers to logfile
    ofstream logfile;
    logfile.open("anomDetectAlert");
  
    if (logfile.is_open())
    {
        // write headings
        logfile << "src/dst" << "\t";
        logfile << "subnet" << "\t";
        logfile << "flowStartMillisec" << "\t";
        logfile << "flowStartSec" << "\t";
        logfile << "-" << "\t";
        logfile << "-" << "\t";
        logfile << endl;
        logfile.close();
    }
    else cout << "Unable to open file";
}


/**
 * Destructor
 */ 
TfdAnomalyDetector::~TfdAnomalyDetector()
{
}


/**
 * Convert IPFIXRecord to Connection Struct
 */
void TfdAnomalyDetector::onDataRecord(IpfixDataRecord* record)
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
void TfdAnomalyDetector::checkConnection(Connection* conn)
{
    //float ivLength = 5;             // length of observation interval (size of timebin)
    uint32_t flowStartSec = 0;      // starttime of flow (seconds)
    uint64_t flowStartMillisec = 0; // starttime of flow (milliseconds)
    uint32_t host = 0;              // host in local network (srcIP or dstIP)
    string hostString;              // host displayed as string
    bool isSrc = false;
    float numFlowPackets;           // number of packets for current flow
    uint32_t srcIP;
    uint32_t dstIP;
    uint16_t srcPort;
    uint16_t dstPort;
    int odPair = 0;
    map<int, PortList>::iterator hostIt;
    
    // check if src or dst of current flow is local host
    if ((conn->srcIP & subnetmask) == subnet) {
        host = conn->srcIP;
        isSrc = true;
        
    } else if ((conn->dstIP & subnetmask) == subnet) {
        host = conn->dstIP;
        isSrc = false;
    }
    
    // get number of packets for current flow
    numFlowPackets = ntohll(conn->srcPackets);
    
    // calc starttime for current flow
    flowStartMillisec = conn->srcTimeStart;
    flowStartSec = (flowStartMillisec + 500) / 1000; // srcTimeStart -rounded- to seconds
 
    // get appropriate timebin for array
    if (((flowStartSec - lastFlowStartSec) >= ivLength) && lastFlowStartSec != 0) {  // todo: variable interval lengths (currently 1 sec) -> ivLength
        timeBin++;      // next timebin
        //cout << "TimeBin: " << timeBin << "\n";
        if (timeBin >= 3) {
          
          /*cout << "\n -- srcIpPackets ---\n";
          for (int m = 0; m < OD_PAIRS; m++){
            cout << "\n";
            cout << "OD-Pair: " << m << "  ";
            for (int n = 0; n < TIMEBINS; n++) {
              cout << srcIpPackets[n][m] << " ";          
            }
          }
          */
        
          calculateEntropy();
          
          /*cout << "\n -- srcIpEntropy ---\n";
          for (int m = 0; m < OD_PAIRS; m++){
            cout << "\n";
            cout << "OD-Pair: " << m << "  ";
            for (int n = 0; n < TIMEBINS; n++) {
              cout << srcIpEntropy[n][m] << " ";          
            }
          }*/
          
          normalizeMatrix(srcIpEntropy, TIMEBINS);
          normalizeMatrix(dstIpEntropy, TIMEBINS);
          normalizeMatrix(srcPortEntropy, TIMEBINS);
          normalizeMatrix(dstPortEntropy, TIMEBINS);
          
          /*cout << "\n -- normalized srcIpEntropy ---\n";
          for (int m = 0; m < OD_PAIRS; m++){
            cout << "\n";
            cout << "OD-Pair: " << m << "  ";
            for (int n = 0; n < TIMEBINS; n++) {
              cout << srcIpEntropy[n][m] << " ";          
            }
          }*/
          
          calculateSingleWayMatrix();
          
          /*cout << "\n -- SingleWayMatrix ---\n";
          for (int m = 0; m < OD_PAIRS*4; m++){
            cout << "\n";
            cout << "OD-Pair: " << m << "  ";
            for (int n = 0; n < TIMEBINS; n++) {
              cout << singleWayMatrix[n][m] << " ";          
            }
          }*/

          /*cout << "\n -- SingleWayMatrix after mean centering ---\n";
          for (int m = 0; m < OD_PAIRS*4; m++){
            cout << "\n";
            cout << "OD-Pair: " << m << "  ";
            for (int n = 0; n < TIMEBINS; n++) {
              cout << singleWayMatrix[n][m] << " ";          
            }
          }*/
          
          calculateExpectationMaximation();

          // restart calculation
          initTempArrays();   // init and delete IP/Port arrays
          timeBin = 0;        // set counter back
        }
        lastFlowStartSec = flowStartSec;
        lastFlowStartMilliSec = flowStartMillisec;
    }
         
    // get OD-value for array
    hostString = IPToString(host);
    int pos;
    for (int i = 0; i < 3; i++) {     // cut host into substrings, only use last part as index for array
      pos = hostString.find(".");
      hostString = hostString.substr(pos+1);
    }
    odPair = atoi(hostString.c_str()); // convert host-substring to int

    // save flowStart to calculate next timebin
    lastFlowStartSec = flowStartSec;
    
    
    // **** save number of packets to IP-Arrays
    if (isSrc) {
        srcIpPackets[timeBin][odPair] += numFlowPackets;
    } else {
        dstIpPackets[timeBin][odPair] += numFlowPackets;
    }
        
    // **** source port
    srcPort = ntohs(conn->srcPort);
    hostIt = srcPortMap.find(odPair);
    if(hostIt != srcPortMap.end()) {
        // host found in Map -> add port to vector
        PortListIterator portIt = find((hostIt->second).begin(), (hostIt->second).end(), srcPort);
        if (portIt != (hostIt->second).end()) {
            //cout << "found srcPort:" << srcPort << "OD: " << odPair << "\n";
        } else {
            //cout << "srcPort: " << srcPort << " doesn't exist for OD: " << odPair <<"\n";
            (hostIt->second).push_back(srcPort);
            srcPortNum[timeBin][odPair] += 1;
        }
    } else {
        // host not yet in Map -> add it
        //cout << " OD: " << odPair << " not yet in map. srcPort: "<< srcPort << "\n";
        // create new port list
        PortList srcPortList(srcPort);
        srcPortMap.insert ( pair<int, PortList>(odPair, srcPortList) );
        srcPortNum[timeBin][odPair] = 1;
    }
    
    // **** destination port
    dstPort = ntohs(conn->dstPort);
    hostIt = dstPortMap.find(odPair);
    if(hostIt != dstPortMap.end()) {
        // host found in Map -> add port to vector
        PortListIterator portIt = find((hostIt->second).begin(), (hostIt->second).end(), dstPort);
        if (portIt != (hostIt->second).end()) {
            //cout << "found dstPort:" << dstPort << " OD: " << odPair << "\n";
        } else {
            //cout << "dstPort: " << dstPort << " doesn't exist for OD: " << odPair <<"\n";
            (hostIt->second).push_back(dstPort);
            dstPortNum[timeBin][odPair] += 1;
        }
    } else {
        // host not yet in Map -> add it
        //cout << " OD: " << odPair << "not yet in map. dstPort: "<< dstPort << "\n";
        // create new port list
        PortList dstPortList(dstPort);
        dstPortMap.insert ( pair<int, PortList>(odPair, dstPortList) );
        dstPortNum[timeBin][odPair] = 1;
    }

}


void TfdAnomalyDetector::calculateEntropy() {
    
    // go to current Entropy value in Entropy matrix
    for(int j=0; j < OD_PAIRS; j++) {
        for(int i=0; i < TIMEBINS; i++) {
        
        // save total observations over all OD-pairs and timebins
        int totalObsSrcIp = 0;
        int totalObsDstIp = 0;
        int totalObsSrcPort = 0;
        int totalObsDstPort = 0;
        
        // save observations for each OD-pair over all timebins
        int singleObsSrcIp[j];
        int singleObsDstIp[j];
        int singleObsSrcPort[j];
        int singleObsDstPort[j];
        
        for (int s=0; s < j+1; s++) {
          singleObsSrcIp[s] = 0;
          singleObsDstIp[s] = 0;
          singleObsSrcPort[s] = 0;
          singleObsDstPort[s] = 0;
        }
        
        
        for(int p=0; p < j+1; p++) {    // OD-Pairs
          for(int t=0; t < i+1; t++) {  // Timebins
              totalObsSrcIp += srcIpPackets[t][p];
              singleObsSrcIp[p]+= srcIpPackets[t][p];
              
              totalObsDstIp += dstIpPackets[t][p];
              singleObsDstIp[p]+= dstIpPackets[t][p];
              
              totalObsSrcPort += srcPortNum[t][p];
              singleObsSrcPort[p]+= srcPortNum[t][p];
              
              totalObsDstPort += dstPortNum[t][p];
              singleObsDstPort[p]+= dstPortNum[t][p];
          }
        }
        
        // calculate srcIpEntropy
        float entropy_val = 0.0;
        if (totalObsSrcIp != 0) {
          for (int k=0; k < j+1; k++) {   // number of distinct values seen in sampled set (OD-Pairs)
            float prob = (float)singleObsSrcIp[k] / (float)totalObsSrcIp;
            if (prob != 0) {
              entropy_val += prob * log(prob);
            } else {
              entropy_val += 0;
            }
           }
          srcIpEntropy[i][j] = - entropy_val;
        } else {
          srcIpEntropy[i][j] = 0.0;
        }
        
        // calculate dstIpEntropy
        entropy_val = 0.0;
        if (totalObsDstIp != 0) {
          for (int k=0; k < j+1; k++) {   // number of distinct values seen in sampled set (OD-Pairs)
            float prob = (float)singleObsDstIp[k] / (float)totalObsDstIp;
            if (prob != 0) {
              entropy_val += prob * log(prob);
            } else {
              entropy_val += 0;
            }
          }
          dstIpEntropy[i][j] = - entropy_val;
        } else {
          dstIpEntropy[i][j] = 0.0;
        }
        
        // calculate srcPortEntropy
        entropy_val = 0.0;
        if (totalObsSrcPort != 0) {
          for (int k=0; k < j+1; k++) {   // number of distinct values seen in sampled set (OD-Pairs)
            float prob = (float)singleObsSrcPort[k] / (float)totalObsSrcPort;
            if (prob != 0) {
              entropy_val += prob * log(prob);
            } else {
              entropy_val += 0;
            }
          }
          srcPortEntropy[i][j] = - entropy_val;
        } else {
          srcPortEntropy[i][j] = 0.0;
        }
        
        // calculate dstPortEntropy
        entropy_val = 0.0;
        if (totalObsDstPort != 0) {
          for (int k=0; k < j+1; k++) {   // number of distinct values seen in sampled set (OD-Pairs)
            float prob = (float)singleObsDstPort[k] / (float)totalObsDstPort;
            if (prob != 0) {
              entropy_val += prob * log(prob);
            } else {
              entropy_val += 0;
            }
          }
          dstPortEntropy[i][j] = - entropy_val;
        } else {
          dstPortEntropy[i][j] = 0.0;
        }
      }
    }
}


void TfdAnomalyDetector::initTempArrays() {
  // init arrays
  for(int i=0; i < TIMEBINS; i++) {
    for(int j=0; j < OD_PAIRS; j++) {
      
        // temp arrays
        srcIpPackets[i][j] = 0;
        dstIpPackets[i][j] = 0;
        srcPortNum[i][j] = 0;
        dstPortNum[i][j] = 0;
    }
  }
  
  // erase port maps
  srcPortMap.erase(srcPortMap.begin(), srcPortMap.end());
  dstPortMap.erase(dstPortMap.begin(), dstPortMap.end());
}


void TfdAnomalyDetector::normalizeMatrix(float matrix[][OD_PAIRS], int timebins) {

  float max = 0.0;
  
  int r,c;
  for(r = 0; r < timebins; ++r) {
    for(c = 0; c < OD_PAIRS; ++c) {
      if(matrix[r][c] > max) {
         max = matrix[r][c];
      }
    }
  }
  for(r = 0; r < timebins; ++r) {
    for(c = 0; c < OD_PAIRS; ++c) {
       if (max!=0) {
          matrix[r][c] = matrix[r][c] / max;
       }
    }
  }
}

void TfdAnomalyDetector::calculateSingleWayMatrix() {

  // init matrix
  for(int i=0; i < TIMEBINS; i++) {
      for(int j=0; j < OD_PAIRS*4; j++) {
        singleWayMatrix[i][j] = 0;
      }
  }
  for(int i=0; i < TIMEBINS; i++) {
      for(int j=0; j < OD_PAIRS; j++) {
        singleWayMatrix[i][j] = srcIpEntropy[i][j];
    }
  }
  for(int i=0; i < TIMEBINS; i++) {
      for(int j=0; j < OD_PAIRS; j++) {
        singleWayMatrix[i][j+OD_PAIRS] = srcPortEntropy[i][j];
    }
  }
  for(int i=0; i < TIMEBINS; i++) {
      for(int j=0; j < OD_PAIRS; j++) {
        singleWayMatrix[i][j+OD_PAIRS*2] = dstIpEntropy[i][j];
    }
  }
  for(int i=0; i < TIMEBINS; i++) {
      for(int j=0; j < OD_PAIRS; j++) {
        singleWayMatrix[i][j+OD_PAIRS*3] = dstPortEntropy[i][j];
    }
  }
}


void TfdAnomalyDetector::calculateExpectationMaximation() {
  float p[OD_PAIRS*4];
  float t[OD_PAIRS*4];
  float row[OD_PAIRS*4];
  float t_abs_last = 0.0;
  
  for (int i=0; i<(OD_PAIRS*4); i++) { // init p and t
          p[i] = 1.0;   // random
  }
  
  // iteration
  for (int iter=0; iter < 10; iter++) {   // max. number of iterations needed for value to convergate
      float xp_temp = 0.0;   // row of singleWayMatrix x * randomVector p
      float t_abs = 0.0;
      
      for (int i=0; i<(OD_PAIRS*4); i++) { // init p and t
          t[i] = 0.0;   // zero
      }
 
      // for each row x in singleWayMatrix
      for (int n=0; n<TIMEBINS; n++) {  

        // (x * p)
        for(int i=0; i < (OD_PAIRS*4); i++) {
            xp_temp += singleWayMatrix[n][i] * p[i];
        }
      
        // (x * p) * x
        for(int i=0; i < (OD_PAIRS*4); i++) {
            row[i] = singleWayMatrix[n][i] * xp_temp;
        }
        
        xp_temp = 0.0;
      
        /*cout << " ROW -------\n";
        for(int i=0; i < (OD_PAIRS*4); i++) {
          cout << row[i] << " "; 
        }*/
      
        // t + (x * p) * x
        for(int i=0; i < (OD_PAIRS*4); i++) {
            t[i] += row[i];
        }
      } // end for each row x in singleWayMatrix
      
      
      // |t|
      for(int i=0; i < (OD_PAIRS*4); i++) {
          t_abs += t[i]*t[i];
      }
      t_abs = sqrt(t_abs);
      
      
      // p = t / |t|
      for(int i=0; i < (OD_PAIRS*4); i++) {
          p[i] = t[i] / t_abs;
      }
      
      //cout << " iter: " << iter << " t_abs: " <<  t_abs << " diff: " <<  (t_abs - t_abs_last) << "\n";
      if ((t_abs - t_abs_last) == 0) {
        break;
      }
      
      t_abs_last = t_abs;
    }
    
    
    float error[TIMEBINS][OD_PAIRS*4];
    // calculate anomalies
    for (int a=0; a<TIMEBINS; a++) {
      float error_val = 0;
        for(int b=0; b < (OD_PAIRS*4); b++) {
           error[a][b] = singleWayMatrix[a][b] - p[b];
           error_val += error[a][b]*error[a][b];  // squared norm
        }
        
        if (error_val > tfdThreshold) {
          // error within timebin detected
          printEntry();
        }
    }
}


void TfdAnomalyDetector::printEntry()
{
    ofstream logfile;
    logfile.open("anomDetectAlert", ios::out | ios::app );
    if (logfile.is_open()) {
        
        logfile << "NET" << "\t";
        logfile << IPToString(subnet).c_str() << "\t";
        logfile << lastFlowStartMilliSec << "\t";
        logfile << lastFlowStartSec << "\t";  // last value used in calculation
        logfile << 0 << "\t";
        logfile << 0 << "\t";
        logfile << endl;
        logfile.close();
        } else {
             cout << "Unable to open file";
        }
}