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

#if !defined(ANOMALYDETECTOR_H)
#define ANOMALYDETECTOR_H

#include "modules/idmef/IDMEFExporter.h"
#include "modules/ipfix/IpfixRecordDestination.h"
#include "modules/ipfix/Connection.h"
#include "core/Source.h"

#include <list>
#include <string>

using namespace std;

class AnomalyDetector 
        : public Module,                  // start/shutdown/config modules
          public IpfixRecordDestination,  // base class for all vermont modules which want to receive flows
	        public Source<IDMEFMessage*>
{
    public:
        AnomalyDetector(uint32_t subnet, uint32_t subnetmask, double packetRateThreshold, double alpha, string analyzerid, string idmeftemplate);
                                
        virtual ~AnomalyDetector();
          
        virtual void onDataRecord(IpfixDataRecord* record);
          
    private:

        struct EmaEntry {
            uint32_t binVal;              // current bin value on time axis (seconds)
            uint64_t flowStartMillisec;   // starttime of flow (milliseconds)
            float numPackets;             // number of packets for current bin
            float ema;                    // EMA for this bin
            float packetRate;             // packetRate for current bin
            bool isSrc;                   // source oder destination
        };

        map<uint32_t, EmaEntry> srcHostMap;  // map for EMA values of outgoing packets (local host is source)
        map<uint32_t, EmaEntry> dstHostMap;  // map for EMA values of incoming packets (local host is destination)
        
        uint32_t subnet;
        uint32_t subnetmask;
        double packetRateThreshold; // packetRate threshold for identification of an attack
        double alpha;               // smoothing factor for EMA
	      string analyzerId;
	      string idmefTemplate;

        static InstanceManager<IDMEFMessage> idmefManager;

        void checkConnection(Connection* conn);
        
        void printEntry(EmaEntry entry, uint32_t host);
};

#endif
