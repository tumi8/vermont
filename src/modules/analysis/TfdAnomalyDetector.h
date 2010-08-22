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

#if !defined(TFDANOMALYDETECTOR_H)
#define TFDANOMALYDETECTOR_H

#include "modules/idmef/IDMEFExporter.h"
#include "modules/ipfix/IpfixRecordDestination.h"
#include "modules/ipfix/Connection.h"
#include "core/Source.h"

#include <list>
#include <string>

#define TIMEBINS 3
#define OD_PAIRS 255


using namespace std;

class TfdAnomalyDetector 
        : public Module,                  // start/shutdown/config modules
          public IpfixRecordDestination,  // base class for all vermont modules which want to receive flows
	        public Source<IDMEFMessage*>
{
    public:
        TfdAnomalyDetector(uint32_t subnet, uint32_t subnetmask, double tfdThreshold, double ivLength, string analyzerid, string idmeftemplate);
                                
        virtual ~TfdAnomalyDetector();
          
        virtual void onDataRecord(IpfixDataRecord* record);
          
    private:
        // arrays for packet counts for each feature
        int srcIpPackets[TIMEBINS][OD_PAIRS];
        int dstIpPackets[TIMEBINS][OD_PAIRS];
        
        // arrays for ports that contains number of distinct ports
        int srcPortNum[TIMEBINS][OD_PAIRS];
        int dstPortNum[TIMEBINS][OD_PAIRS];
        
        // arrays for entropy values for each feature
        float srcIpEntropy[TIMEBINS][OD_PAIRS];
        float dstIpEntropy[TIMEBINS][OD_PAIRS];
        float srcPortEntropy[TIMEBINS][OD_PAIRS];
        float dstPortEntropy[TIMEBINS][OD_PAIRS];
        
        float singleWayMatrix[TIMEBINS][OD_PAIRS*4];
        
   
        uint32_t lastFlowStartSec;
        uint64_t lastFlowStartMilliSec;
        int timeBin;      // 0 bis 2
        
        typedef vector<int> PortList;
        typedef PortList::iterator PortListIterator;
        
        map<int, PortList> srcPortMap;    // key = last part of host (OD-Pairs)
        map<int, PortList> dstPortMap;    // key = last part of host (OD-Pairs)
        
        uint32_t subnet;
        uint32_t subnetmask;
	      string analyzerId;
	      string idmefTemplate;
	      double tfdThreshold;
	      double ivLength;

        static InstanceManager<IDMEFMessage> idmefManager;

        void checkConnection(Connection* conn);
        
        void normalizeMatrix(float matrix[][OD_PAIRS], int timebins);
        
        void calculateSingleWayMatrix();
        
        void calculateEntropy();
        
        void initTempArrays();
        
        void calculateExpectationMaximation();

        void printEntry();
};

#endif